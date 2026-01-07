import pandas as pd
import numpy as np
import os
from scipy.signal import find_peaks
# import matplotlib.pyplot as plt

# 1. File and directory setup
folder_path = r"C:\\Users\\wujik\\Documents\\KLTN\Data\\ecg_data\\rapid\\"
prefix = "ecg_rapid"
file_index = 1

# 2. Parameters and initialization
segments = []
segment_per_file = []
fs = 125                # Sampling frequency
segment_length = 188    # Length of each ECG segment

# 3. Read and process ECG files
while True:
    file_path = os.path.join(folder_path, f"{prefix}_{file_index}.csv")
    if not os.path.exists(file_path):
        break

    # Load ECG data
    ecg = pd.read_csv(file_path, header=None).iloc[:, 0].values

    # Peak R detection function
    ecg_norm = (ecg - np.mean(ecg)) / (np.std(ecg) + 1e-8)
    peaks, _ = find_peaks(
        ecg_norm,
        height=4,            # thấp hơn để không miss đỉnh R
        distance=int(fs * 0.6),# R-R tối thiểu 0.5s
        prominence=4,        # R-peaks luôn nổi bật
        width=(1, 12)          # R nhọn, T rộng → tránh nhầm T
    )

    seg_count = 0

    # 4. Segment extraction
    for i in range(len(peaks) - 1):
        start = peaks[i]
        next_r = peaks[i + 1]
        
        # Get more 20% after next R-peak
        end = start + int(1.2 * (next_r - start))
        end = min(end, len(ecg))

        segment = ecg[start:end].astype(float)

        segment_min, segment_max = segment.min(), segment.max()
        if segment_max > segment_min:
            segment = (segment - segment_min) / (segment_max - segment_min)
        else:
            segment = np.zeros_like(segment)

        if len(segment) < segment_length:
            segment = np.pad(segment, (0, segment_length - len(segment) - 1))
        else:
            segment = segment[:segment_length - 1]

        segments.append(segment)
        seg_count += 1

    segment_per_file.append(seg_count)
    print(f"File {file_index}: extracted {seg_count} segments.")
    print(f"File {file_index}: detected {len(peaks)} R-peaks.")
    
    file_index += 1

segments = np.array(segments)

# 5. Labeling segments (0 = normal, 1 = abnormal)
labels = np.zeros((len(segments), 1))
data_out = np.hstack((segments, labels))

# 6. Save processed data to CSV
output_file_path = os.path.join(folder_path, f"processed_{prefix}_test_2.csv")
pd.DataFrame(data_out).to_csv(output_file_path, index=False, header=False)
print("Processed data saved to:")
print(output_file_path)

# # Visualization (optional)
# while True:
#     try:
#         # Ask the user which segment to view
#         segment_index = int(input(f"Enter the segment number to view (1–{len(segments)}), or 0 to exit: "))

#         # Exit the program if user enters 0
#         if segment_index == 0:
#             print("Program terminated.")
#             break

#         # Check if the input is valid
#         if 1 <= segment_index <= len(segments):
#             segment = segments[segment_index - 1]  # list indices start at 0
#             time = np.arange(len(segment)) / fs  # time in seconds

#             plt.figure(figsize=(8, 3))
#             plt.plot(time, segment, linewidth=1)
#             plt.title(f"Segment {segment_index} (length={len(segment)} samples)")
#             plt.xlabel("Time (s)")
#             plt.ylabel("Normalized Amplitude")
#             plt.grid(True)
#             plt.tight_layout()
#             plt.show()

#         else:
#             print(f"The number you entered ({segment_index}) is out of range (1–{len(segments)}).")

#     except ValueError:
#         print("Please enter a valid integer.")
