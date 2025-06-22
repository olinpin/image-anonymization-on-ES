# Anonymization of Images for Privacy Protection on Embedded Systems

[![ESP32-P4][esp32p4-badge]][esp32p4-url] [![ESP-IDF v5.4][espidf-badge]][espidf-url] [![License: MIT][license-badge]][license-url]

This repository contains the implementation for the thesis **"Anonymization of Images for Privacy Protection on Embedded Systems"**, demonstrating face detection and anonymization techniques on ESP32-P4 microcontroller using ESP-DL framework.

## 🎯 Project Overview

This project implements three different face anonymization techniques on embedded hardware:

- **Pixelation**: Reduces image resolution in face regions
- **Blurring**: Applies Gaussian blur to detected faces
- **Blackout**: Complete removal of face regions with black rectangles

The system processes a sample image (`human_face.jpg`) and applies anonymization on the ESP32-P4 microcontroller. With ~20ms processing time per frame, the system is fast enough to potentially support real-time camera applications with modifications.

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Input Image   │───▶│ JPEG Decoding    │───▶│ Preprocessing   │
│  human_face.jpg │    │ & RGB Conversion │    │ & Resize        │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                                        │
                                                        ▼
                       ┌──────────────────┐    ┌─────────────────┐
                       │ Face Detection   │◀───│ MSR Model       │
                       │ Results          │    │ Inference       │
                       └──────────────────┘    └─────────────────┘
                                │
                                ▼
                       ┌──────────────────┐    ┌─────────────────┐
                       │ Bounding Boxes   │───▶│  Anonymization  │
                       │ & Confidence     │    │   Techniques    │
                       └──────────────────┘    └─────────────────┘
```

## 🚀 Features

- **Fast Face Detection**: Using ESP-DL's MSR (Multi-Scale Region) detection model
- **Multiple Anonymization Methods**: Pixelation, blurring, and blackout techniques
- **Embedded Optimization**: Memory-efficient implementation for resource-constrained devices
- **Configurable Parameters**: Adaptive blur/pixelation sizes based on face dimensions
- **Performance Monitoring**: Detailed timing analysis for each processing stage

## 📋 Requirements

- **ESP32-P4** development board
- **ESP-IDF v5.4+**
- **ESP-DL v3.1.3+** (managed component)
- Python 3.x (for image decoding utilities)

## 🛠️ Installation & Setup

### 1. Clone Repository

```bash
git clone <repository-url>
cd image-anonymization-on-ESP
```

### 2. Set up ESP-IDF Environment

```bash
# Install ESP-IDF v5.4
. $HOME/esp/esp-idf/export.sh

# Verify installation
idf.py --version
```

### 3. Configure Target

```bash
idf.py set-target esp32p4
```

### 4. Install Dependencies

```bash
# Dependencies are automatically managed through idf_component.yml
idf.py reconfigure
```

## ⚙️ Configuration

### Model Configuration

- **Detection Model**: MSR (Multi-Scale Region) with S8 quantization
- **Input Resolution**: 120×160×3 (RGB)
- **Confidence Threshold**: 0.5
- **NMS Threshold**: 0.5

### Anonymization Parameters

```cpp
// Pixelation: 10% of face dimension (minimum 3px)
int pixelate_size = (min_face_dimension * 10) / 100;

// Blur: 15% of face dimension (minimum 15px, max 1/3 face size)
int blur_size = (min_face_dimension * 15) / 100;

// Iterations: 3 blur passes for optimal effect
int blur_iterations = 3;
```

## 🔧 Building and Flashing

### Build Project

```bash
idf.py build
```

### Flash to Device

```bash
idf.py flash monitor
```

### Expected Output

```
I (955) human_face_detect: [score: 0.936285, x1: 100, y1: 64, x2: 193, y2: 191]
I (955) MSR: Preprocess time: 5732 us
I (955) MSR: Model run time: 14328 us
I (955) MSR: Postprocess time: 118 us
I (955) MSR: Total detection: 20178 us, found 1 faces
I (955) DETECTION: Found 1 faces
```

## 📊 Performance Analysis

### Timing Breakdown (ESP32-P4 @ 400MHz)

| Stage           | Time (μs)   | Percentage |
| --------------- | ----------- | ---------- |
| Preprocessing   | ~5,700      | 28.5%      |
| Model Inference | ~14,200     | 71.0%      |
| Postprocessing  | ~100        | 0.5%       |
| Anonymization   | ~2          | 0.01%      |
| **Total**       | **~20,000** | **100%**   |

## 🖼️ Image Processing Pipeline

### 1. Input Processing

- JPEG decoding from embedded binary
- RGB888 color space conversion
- Memory allocation in PSRAM

### 2. Face Detection

- Multi-scale region proposal
- Deep learning inference (S8 quantized)
- Non-maximum suppression
- Confidence filtering

### 3. Anonymization

Three techniques implemented:

#### Pixelation

```cpp
void pixelate(dl::image::img_t *img, int x1, int y1, int x2, int y2) {
    // Calculate adaptive pixel size
    // Average colors in blocks
    // Apply uniform color to block
}
```

#### Blurring

```cpp
void blur_out(dl::image::img_t *img, int x1, int y1, int x2, int y2) {
    // Multi-iteration Gaussian-like blur
    // Adaptive kernel size
    // Edge-aware processing
}
```

#### Blackout

```cpp
void black_out(dl::image::img_t *img, int x1, int y1, int x2, int y2) {
    // Complete pixel replacement
    // Zero RGB values
}
```

## 📁 Project Structure

```
├── main/
│   ├── app_main.cpp          # Main application logic
│   ├── human_face.jpg        # Sample input image
│   ├── CMakeLists.txt        # Build configuration
│   └── idf_component.yml     # Dependencies
├── CMakeLists.txt            # Root build file
├── sdkconfig.defaults        # Default configuration
├── partitions.csv            # Flash partition layout
├── decode.py                 # Python image decoder
├── main.py                   # Offline analysis tools
└── README.md                 # This file
```

## 🔬 Testing & Validation

### Offline Analysis

Use `main.py` for comprehensive privacy protection analysis:

```bash
python main.py
```

This script evaluates protection effectiveness using DeepFace recognition:

- **Face Recognition Rate**: Measures anonymization success
- **Multiple Detection Models**: Facenet, VGG-Face, etc.
- **Similarity Thresholds**: Configurable recognition sensitivity

### Image Extraction

To extract processed images from device output:

```bash
# Capture device output
idf.py monitor > output.txt

# Decode pixel data to PNG
python decode.py
```

## 🔧 Customization

### Changing Anonymization Method

Modify the detection task in `app_main.cpp`:

```cpp
// Replace pixelate() with desired method
for (const auto &res : detect_results) {
    blur_out(&args->img, res.box[0], res.box[1], res.box[2], res.box[3]);
    // or black_out(&args->img, res.box[0], res.box[1], res.box[2], res.box[3]);
}
```

### Adjusting Detection Sensitivity

```cpp
// In MSRPostprocessor constructor
postprocessor = new dl::detect::MSRPostprocessor(
    model,
    0.3,  // Lower = more sensitive detection
    0.5,  // NMS threshold
    10,   // Max detections
    // ... anchor configurations
);
```

### Custom Input Images

Replace `human_face.jpg` in the `main/` directory and rebuild:

```bash
idf.py build flash
```

## 📚 Research Context

This implementation serves as a practical demonstration of:

- **Edge AI Computing**: Deep learning inference on microcontrollers
- **Privacy-Preserving Computer Vision**: Embedded anonymization techniques
- **Optimized Image Processing**: Efficient algorithms for resource constraints
- **IoT Security**: Privacy protection at the edge

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Espressif Systems** for ESP-DL framework and ESP32-P4 platform
- **Academic Supervisors** for guidance and support

---

[esp32p4-badge]: https://img.shields.io/badge/ESP32-P4-red
[esp32p4-url]: https://www.espressif.com/en/products/socs/esp32-p4
[espidf-badge]: https://img.shields.io/badge/ESP--IDF-v5.4-blue
[espidf-url]: https://docs.espressif.com/projects/esp-idf/en/latest/
[license-badge]: https://img.shields.io/badge/License-MIT-yellow.svg
[license-url]: https://opensource.org/licenses/MIT

**⚡ Bringing Privacy Protection to the Edge with ESP32-P4**
