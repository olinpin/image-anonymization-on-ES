#include "dl_detect_msr_postprocessor.hpp"
#include "dl_image_define.hpp"
#include "dl_image_preprocessor.hpp"
#include "dl_model_base.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include <cstdint>
#include <list>
#include <string>

extern const uint8_t human_face_jpg_start[] asm("_binary_human_face_jpg_start");
extern const uint8_t human_face_jpg_end[] asm("_binary_human_face_jpg_end");
extern const uint8_t human_face_detect_msr_s8_v1_espdl_start[] asm(
    "_binary_human_face_detect_msr_s8_v1_espdl_start");
extern const uint8_t human_face_detect_mnp_s8_v1_espdl_start[] asm(
    "_binary_human_face_detect_mnp_s8_v1_espdl_start");
const char *TAG = "human_face_detect";

struct Color {
  uint8_t r, g, b;
  Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) : r(r), g(g), b(b) {}
};

void set_pixel(dl::image::img_t *img, int x, int y, const Color &color) {
  if (!img || x < 0 || y < 0 || x >= img->width || y >= img->height)
    return;

  uint8_t *data = (uint8_t *)img->data;
  size_t index = (y * img->width + x) * 3;

  data[index + 0] = color.r;
  data[index + 1] = color.g;
  data[index + 2] = color.b;
}

Color get_pixel(const dl::image::img_t *img, int x, int y) {
  if (!img || x < 0 || y < 0 || x >= img->width || y >= img->height)
    return Color();

  const uint8_t *data = (const uint8_t *)img->data;
  size_t index = (y * img->width + x) * 3;

  return Color(data[index + 0], data[index + 1], data[index + 2]);
}

void print_picture(dl::image::img_t img) {
  printf("\n===PIXELS_START===\n");
  for (size_t i = 0; i < img.width * img.height * 3; ++i) {
    printf("%02X", ((uint8_t *)img.data)[i]);

    if ((i + 1) % 48 == 0)
      printf("\n");

    if ((i + 1) % 1024 == 0) {
      fflush(stdout);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  printf("\n===PIXELS_END===\n");
}

void black_out(dl::image::img_t *img, int x1, int y1, int x2, int y2) {
  for (int y = y1; y < y2; ++y) {
    for (int x = x1; x < x2; ++x) {
      set_pixel(img, x, y, Color(0, 0, 0));
    }
  }
}

void blur_out(dl::image::img_t *img, int x1, int y1, int x2, int y2) {
  // default blur size and iterations
  int blur_size = 20;
  int blur_iterations = 3;

  // blur size should be 15% of the face size - trial and error
  int face_width = x2 - x1;
  int face_height = y2 - y1;
  int min_face_dimension =
      (face_width < face_height) ? face_width : face_height;

  blur_size = (min_face_dimension * 15) / 100;
  if (blur_size < 15) {
    blur_size = 15;
  }
  // blur size should not exceed 1/3 of the face size - makes the face too blury
  if (blur_size > min_face_dimension / 3) {
    blur_size = min_face_dimension / 3;
  }

  // do the actual blurring + iterate X many times
  for (int iteration = 0; iteration < blur_iterations; ++iteration) {
    for (int y = y1; y < y2; ++y) {
      for (int x = x1; x < x2; ++x) {
        int r_sum = 0, g_sum = 0, b_sum = 0;
        int count = 0;

        for (int dy = -blur_size; dy <= blur_size; ++dy) {
          for (int dx = -blur_size; dx <= blur_size; ++dx) {
            int nx = x + dx;
            int ny = y + dy;

            if (nx >= 0 && ny >= 0 && nx < img->width && ny < img->height) {
              Color color = get_pixel(img, nx, ny);
              r_sum += color.r;
              g_sum += color.g;
              b_sum += color.b;
              count++;
            }
          }
        }

        if (count > 0) {
          r_sum /= count;
          g_sum /= count;
          b_sum /= count;
          set_pixel(img, x, y, Color(r_sum, g_sum, b_sum));
        }
      }
    }
  }
}

void pixelate(dl::image::img_t *img, int x1, int y1, int x2, int y2) {
  int pixelate_size = 3;
  int face_width = x2 - x1;
  int face_height = y2 - y1;
  int min_face_dimension =
      (face_width < face_height) ? face_width : face_height;
  pixelate_size = (min_face_dimension * 10) / 100;
  if (pixelate_size < 3) {
    pixelate_size = 3;
  }

  for (int y = y1; y < y2; y += pixelate_size) {
    for (int x = x1; x < x2; x += pixelate_size) {
      int r_sum = 0, g_sum = 0, b_sum = 0;
      int count = 0;

      // calculate the average color of the square
      for (int dy = 0; dy < pixelate_size; ++dy) {
        for (int dx = 0; dx < pixelate_size; ++dx) {
          int nx = x + dx;
          int ny = y + dy;

          if (nx >= 0 && ny >= 0 && nx < img->width && ny < img->height) {
            Color color = get_pixel(img, nx, ny);
            r_sum += color.r;
            g_sum += color.g;
            b_sum += color.b;
            count++;
          }
        }
      }

      if (count > 0) {
        r_sum /= count;
        g_sum /= count;
        b_sum /= count;

        // set all pixels in the square to the average color
        for (int dy = 0; dy < pixelate_size; ++dy) {
          for (int dx = 0; dx < pixelate_size; ++dx) {
            int nx = x + dx;
            int ny = y + dy;

            if (nx >= 0 && ny >= 0 && nx < img->width && ny < img->height) {
              set_pixel(img, nx, ny, Color(r_sum, g_sum, b_sum));
            }
          }
        }
      }
    }
  }
}

void print_subpicture(dl::image::img_t img, int x1, int y1, int x2, int y2) {
  printf("\n===PIXELS_START===\n");
  for (int y = y1; y < y2; ++y) {
    for (int x = x1; x < x2; ++x) {
      Color color = get_pixel(&img, x, y);
      printf("%02X%02X%02X ", color.r, color.g, color.b);
    }
    printf("\n");
  }
  printf("===PIXELS_END===\n");
  printf("Resolution: %d x %d\n", img.width, img.height);
}

// Model configuration
#if CONFIG_HUMAN_FACE_DETECT_MODEL_IN_FLASH_RODATA
extern const uint8_t
    human_face_detect_espdl[] asm("_binary_human_face_detect_espdl_start");
static const char *model_path = (const char *)human_face_detect_espdl;
#elif CONFIG_HUMAN_FACE_DETECT_MODEL_IN_FLASH_PARTITION
static const char *model_path = "human_face_det";
#else
#if !defined(CONFIG_BSP_SD_MOUNT_POINT)
#define CONFIG_BSP_SD_MOUNT_POINT "/sdcard"
#endif
#endif

// Static variable definitions for MSRDetect
uint8_t *MSRDetect_resize_buffer = nullptr;
size_t MSRDetect_resize_buffer_size = 0;
bool MSRDetect_buffers_initialized = false;

class MSRDetect {
public:
  dl::Model *model;
  dl::image::ImagePreprocessor *preprocessor;
  dl::detect::MSRPostprocessor *postprocessor;
  uint8_t *custom_input_buffer;

  MSRDetect(const char *model_name) {
    model = new dl::Model((const char *)human_face_detect_msr_s8_v1_espdl_start,
                          model_name, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    model->minimize();
    preprocessor = new dl::image::ImagePreprocessor(model, {0, 0, 0}, {1, 1, 1},
                                                    DL_IMAGE_CAP_RGB_SWAP);
    postprocessor = new dl::detect::MSRPostprocessor(
        model, 0.5, 0.5, 10,
        {{8, 8, 9, 9, {{16, 16}, {32, 32}}},
         {16, 16, 9, 9, {{64, 64}, {128, 128}}}});

    // Allocate buffer for custom preprocessing (120x160x3)
    custom_input_buffer =
        (uint8_t *)heap_caps_malloc(120 * 160 * 3, MALLOC_CAP_8BIT);
    if (custom_input_buffer) {
      ESP_LOGI("MSR", "Custom preprocessing buffer allocated: %d bytes",
               120 * 160 * 3);
    } else {
      ESP_LOGE("MSR", "Failed to allocate custom preprocessing buffer");
    }

    if (!MSRDetect_buffers_initialized) {
      MSRDetect_resize_buffer_size = 120 * 160 * 3;
      MSRDetect_resize_buffer = (uint8_t *)heap_caps_malloc(
          MSRDetect_resize_buffer_size, MALLOC_CAP_8BIT);
      if (MSRDetect_resize_buffer) {
        ESP_LOGI("MSR", "Pre-allocated resize buffer: %d bytes",
                 MSRDetect_resize_buffer_size);
        MSRDetect_buffers_initialized = true;
      } else {
        ESP_LOGW("MSR", "Failed to pre-allocate resize buffer");
      }
    }
  }

  ~MSRDetect() {
    if (custom_input_buffer) {
      heap_caps_free(custom_input_buffer);
    }
    delete postprocessor;
    delete preprocessor;
    delete model;
  }

  std::list<dl::detect::result_t> run(const dl::image::img_t &img) {

    int64_t preprocess_start = esp_timer_get_time();
    preprocessor->preprocess(img);
    int64_t preprocess_end = esp_timer_get_time();

    int64_t model_start = esp_timer_get_time();
    model->run();
    int64_t model_end = esp_timer_get_time();

    int64_t postprocess_start = esp_timer_get_time();
    postprocessor->clear_result();
    postprocessor->set_resize_scale_x(preprocessor->get_resize_scale_x());
    postprocessor->set_resize_scale_y(preprocessor->get_resize_scale_y());
    postprocessor->postprocess();
    std::list<dl::detect::result_t> &result =
        postprocessor->get_result(img.width, img.height);
    int64_t postprocess_end = esp_timer_get_time();

    ESP_LOGI("MSR", "Preprocess time: %lld us",
             preprocess_end - preprocess_start);
    ESP_LOGI("MSR", "Model run time: %lld us", model_end - model_start);
    ESP_LOGI("MSR", "Postprocess time: %lld us",
             postprocess_end - postprocess_start);
    ESP_LOGI("MSR", "Total detection: %lld us, found %d faces",
             postprocess_end - preprocess_start, result.size());

    return result;
  }
};

class ESPDLFaceDetect {
private:
  MSRDetect *msr_detect;

public:
  ESPDLFaceDetect() {
    ESP_LOGI(TAG, "Initializing ESP-DL Face Detection");
    msr_detect = new MSRDetect("human_face_detect_msr_s8_v1.espdl");
    ESP_LOGI(TAG, "ESP-DL Face Detection initialized successfully");
  }

  ~ESPDLFaceDetect() {
    delete msr_detect;
    ESP_LOGI(TAG, "ESP-DL Face Detection destroyed");
  }

  std::list<dl::detect::result_t> run(const dl::image::img_t &img) {
    return msr_detect->run(img);
  }
};

typedef struct {
  dl::image::img_t img;
  ESPDLFaceDetect *detect;
  int task_id;
} run_detection_args;

void run_detection_task(void *pvParameters) {
  run_detection_args *args = (run_detection_args *)pvParameters;

  auto detect_results = args->detect->run(args->img);

  ESP_LOGI("DETECTION", "Found %d faces", detect_results.size());
  for (const auto &res : detect_results) {
    ESP_LOGI("DETECTION", "[score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
             res.score, res.box[0], res.box[1], res.box[2], res.box[3]);
  }

  for (const auto &res : detect_results) {
    pixelate(&args->img, res.box[0], res.box[1], res.box[2], res.box[3]);
  }

  vTaskDelete(NULL);
}

extern "C" void app_main(void) {
  dl::image::jpeg_img_t jpeg_img = {
      .data = (void *)human_face_jpg_start,
      .data_len = (size_t)(human_face_jpg_end - human_face_jpg_start)};
  auto img = sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);

  vTaskDelay(pdMS_TO_TICKS(1000));

  ESPDLFaceDetect *detect = new ESPDLFaceDetect();

  run_detection_args *args = new run_detection_args;
  args->img = img;
  args->detect = detect;
  args->task_id = 1;

  xTaskCreate(run_detection_task, "DetectionTask", 4096, args, 5, NULL);
  vTaskDelay(pdMS_TO_TICKS(1000));

  delete detect;
  heap_caps_free(img.data);
}
