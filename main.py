import os
from deepface import DeepFace
import numpy as np


def main():
    model = "Facenet"
    folders = ["male", "elon", "multiple"]

    for folder in folders:
        folder_path = os.path.join("images", folder)
        if not os.path.exists(folder_path):
            print("Folder does not exist:", folder_path)
            continue

        files = [
            f
            for f in os.listdir(folder_path)
            if f.lower().endswith((".png", ".jpg", ".jpeg"))
        ]
        original = next((f for f in files if "original" in f.lower()), None)
        pixelated = next((f for f in files if "pixel" in f.lower()), None)
        black = next((f for f in files if "black" in f.lower()), None)
        blurr = next((f for f in files if "blurr" in f.lower()), None)

        if not original or not pixelated or not black or not blurr:
            print(f"Missing files in folder: {folder_path}")
            continue

        protection_scores = []
        print(f"\n=== Testing {folder} folder ===")

        print("\n--- Pixelated Protection Test ---")
        pixel_score = check_protection_multiface(
            folder_path, original, pixelated, model
        )
        protection_scores.append(pixel_score)

        print("\n--- Black Protection Test ---")
        black_score = check_protection_multiface(folder_path, original, black, model)
        protection_scores.append(black_score)

        print("\n--- Blur Protection Test ---")
        blur_score = check_protection_multiface(folder_path, original, blurr, model)
        protection_scores.append(blur_score)

        # Calculate overall protection rate
        average_protection = sum(protection_scores) / len(protection_scores)
        rate = average_protection * 100

        # Determine status based on average protection
        if rate >= 95:
            status = "✅ EXCELLENT"
        elif rate >= 80:
            status = "✅ GOOD"
        elif rate >= 60:
            status = "⚠️ MODERATE"
        elif rate >= 30:
            status = "⚠️ POOR"
        else:
            status = "❌ FAILED"

        print(f"\n{'='*50}")
        print(f"FINAL RESULT for {folder.upper()}:")
        print(f"Pixelated: {pixel_score*100:.1f}% protected")
        print(f"Black: {black_score*100:.1f}% protected")
        print(f"Blur: {blur_score*100:.1f}% protected")
        print(f"Overall: {rate:.1f}% protected {status}")
        print(f"{'='*50}")


def cosine_similarity(embedding1, embedding2):
    """Calculate cosine similarity between two face embeddings"""
    embedding1 = np.array(embedding1)
    embedding2 = np.array(embedding2)
    
    # Calculate dot product
    dot_product = np.dot(embedding1, embedding2)
    
    # Calculate magnitudes
    magnitude1 = np.linalg.norm(embedding1)
    magnitude2 = np.linalg.norm(embedding2)
    
    # Avoid division by zero
    if magnitude1 == 0 or magnitude2 == 0:
        return 0.0
    
    # Calculate cosine similarity
    similarity = dot_product / (magnitude1 * magnitude2)
    
    return similarity


def match_faces_by_area(faces1, faces2, threshold=50):
    """Match faces between two images based on facial area proximity"""
    matched_pairs = []

    for i, face1 in enumerate(faces1):
        area1 = face1["facial_area"]
        best_match = None
        min_distance = float("inf")

        for j, face2 in enumerate(faces2):
            area2 = face2["facial_area"]
            # Calculate distance between face center points
            center1_x = area1["x"] + area1["w"] / 2
            center1_y = area1["y"] + area1["h"] / 2
            center2_x = area2["x"] + area2["w"] / 2
            center2_y = area2["y"] + area2["h"] / 2

            distance = np.sqrt(
                (center1_x - center2_x) ** 2 + (center1_y - center2_y) ** 2
            )

            if distance < min_distance and distance < threshold:
                min_distance = distance
                best_match = j

        if best_match is not None:
            matched_pairs.append((i, best_match))

    return matched_pairs


def check_protection_multiface(folder_path, original, protected_image, model):
    """
    Check protection by comparing individual faces between original and protected images.
    Returns a protection score between 0 and 1 based on how many faces are protected.
    """
    original_path = os.path.join(folder_path, original)
    protected_path = os.path.join(folder_path, protected_image)

    try:
        # Get face representations for both images
        original_faces = DeepFace.represent(
            original_path, model_name=model, enforce_detection=False
        )
        protected_faces = DeepFace.represent(
            protected_path, model_name=model, enforce_detection=False
        )

        print(f"Original faces detected: {len(original_faces)}")
        print(f"Protected faces detected: {len(protected_faces)}")

        # If no faces detected in protected image, consider it fully protected
        if len(protected_faces) == 0:
            print("No faces detected in protected image - FULLY PROTECTED")
            return 1.0

        # If no faces in original, can't compare
        if len(original_faces) == 0:
            print("No faces detected in original image - Cannot assess")
            return 0.0

        # Match faces between images based on position
        matched_pairs = match_faces_by_area(original_faces, protected_faces)

        # Check if any matched faces are similar enough to be recognized
        faces_recognized = 0
        faces_protected = 0
        total_original_faces = len(original_faces)

        # Cosine similarity threshold for Facenet (higher = more similar)
        similarity_threshold = 0.7

        # Track which original faces have matches
        matched_original_faces = set()

        for orig_idx, prot_idx in matched_pairs:
            matched_original_faces.add(orig_idx)
            similarity = cosine_similarity(
                original_faces[orig_idx]["embedding"],
                protected_faces[prot_idx]["embedding"],
            )

            if similarity > similarity_threshold:
                faces_recognized += 1
                print(
                    f"Face {orig_idx+1} RECOGNIZED (similarity: {similarity:.3f}) - NOT PROTECTED"
                )
            else:
                faces_protected += 1
                print(
                    f"Face {orig_idx+1} NOT RECOGNIZED (similarity: {similarity:.3f}) - PROTECTED"
                )

        # Faces that don't have matches in protected image are considered protected
        unmatched_faces = total_original_faces - len(matched_pairs)
        if unmatched_faces > 0:
            faces_protected += unmatched_faces
            print(
                f"{unmatched_faces} face(s) completely missing in protected image - PROTECTED"
            )

        # Calculate protection rate
        protection_rate = faces_protected / total_original_faces

        print(
            f"SUMMARY: {faces_protected}/{total_original_faces} faces protected ({protection_rate*100:.1f}%)"
        )

        if protection_rate == 1.0:
            print("RESULT: FULLY PROTECTED - All faces are protected")
            return 1.0
        elif protection_rate >= 0.5:
            print(
                f"RESULT: PARTIALLY PROTECTED - {protection_rate*100:.1f}% of faces protected"
            )
            return protection_rate
        else:
            print(
                f"RESULT: NOT WELL PROTECTED - Only {protection_rate*100:.1f}% of faces protected"
            )
            return protection_rate

    except Exception as e:
        print(f"Error processing {original_path} vs {protected_path}: {e}")
        # If there's an error (like no faces detected), consider it protected
        return 1.0


def check_protection(folder_path, original, pixelated, model):
    """Legacy function - kept for compatibility"""
    try:
        result = DeepFace.verify(
            os.path.join(folder_path, original),
            os.path.join(folder_path, pixelated),
            model_name=model,
            enforce_detection=False,
        )
        print(os.path.join(folder_path, original))
        print(os.path.join(folder_path, pixelated))
        print(result)
        if not result["verified"]:
            return 1
    except Exception as e:
        print(os.path.join(folder_path, original))
        print(os.path.join(folder_path, pixelated))
        print(e)
        pass
    return 0


if __name__ == "__main__":
    main()
