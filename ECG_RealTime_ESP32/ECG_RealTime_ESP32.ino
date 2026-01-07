// ===== LIBRARIES DECLARE =====
#include <Arduino.h>
#include "esp_timer.h"
#include <math.h>
#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include "model_data.h"     // g_model[], g_model_len
#include "scaler_data.h"    // scaler_mean[187], scaler_scale[187]

// ====== HARDWARE CONFIG ======
#define ADC_PIN   1
#define LO_POS    42
#define LO_NEG    41
#define GRN_LED   4
#define RED_LED   5
#define VREF      3.3f
#define ADC_BITS  12

// ====== MODEL CONFIG ======
#define NUM_INPUTS   187
#define NUM_OUTPUTS  2
#define ARENA_SIZE   (200 * 1024)   // tăng nếu báo lỗi AllocateTensors
#define TF_NUM_OPS   16             // Số ops tối đa mà model dùng (Conv2D, MaxPool2D, FC, Softmax, Reshape, Relu...)

// Sequential model (TinyML)
Eloquent::TF::Sequential<TF_NUM_OPS, ARENA_SIZE> tf;

// buffer đưa vào model sau chuẩn hoá
float input_buffer[NUM_INPUTS];

// ===== TIMER FLAG =====
volatile bool sampleFlag = false;
// ISR: called every 8 ms (125 Hz)
void IRAM_ATTR onTimer(void* arg) {
    sampleFlag = true;
}

// ===== CONSTANT =====
const int samples = 125*4;

// ===== BIẾN TOÀN CỤC CHO PHÁT HIỆN ĐỈNH R =====
uint16_t    ecg_raw[samples];
float       ecg_norm[samples];
float       ecg_mean;
float       ecg_std;
uint16_t    peaks[2];
uint16_t    start;
uint16_t    end;
float       segment[NUM_INPUTS];
float       seg_min;
float       seg_max;

// ===== NP.MEAN FUNCTION IN C =====
float mean(const uint16_t *arr, int length) {
    if (length <= 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < length; i++) {
        sum += arr[i];
    }
    return sum / (float)length;
}

// ===== NP.STD FUNCTION IN C =====
float npstd(const uint16_t *arr, int length) {
    if (length <= 0) return 0.0f;
    float m = mean(arr, length);
    float sum_sq = 0.0f;
    for (int i = 0; i < length; i++) {
        float diff = arr[i] - m;
        sum_sq += diff * diff;
    }
    return sqrtf(sum_sq / (float)length);
}

// ===== FIND TWO PEAKS =====
uint8_t find_two_peaks(const float *signal, int n, float height, int distance, uint16_t peaks[2]) {
    int last_peak_index = -distance;
    uint8_t count = 0;

    for (int i = 1; i < n - 1; i++){
        if (signal[i] > signal[i - 1] && signal[i] > signal[i + 1]){
            if (signal[i] >= height) {
                if (i - last_peak_index >= distance) {
                    peaks[count] = (uint16_t)i;
                    count++;
                    last_peak_index = i;
                    if (count == 2)     break;
                }
            }
        }
    }
    return count;
}

// ===== FIND MINIMUM IN AN ARRAY =====
float array_min(const float *arr, int length) {
    if (length <= 0) return 0.0f;

    float min_val = arr[0];
    for (int i = 1; i < length; i++) {
        if (arr[i] < min_val) {
            min_val = arr[i];
        }
    }
    return min_val;
}

// ===== FIND MAXIMUM IN AN ARRAY =====
float array_max(const float *arr, int length) {
    if (length <= 0) return 0.0f;

    float max_val = arr[0];
    for (int i = 1; i < length; i++) {
        if (arr[i] > max_val) {
            max_val = arr[i];
        }
    }
    return max_val;
}

// ===== ACQUIRE ECG SIGNAL =====
void acquire_window() {
    int num_sample = 0;
    while (num_sample < samples) {
        if (sampleFlag) {
            sampleFlag = false;
            ecg_raw[num_sample] = analogRead(ADC_PIN);
            num_sample++;
        }
    }
}

// ===== CHUẨN HOÁ CHO MODEL (giống StandardScaler bên Colab) =====
void normalize_segment_for_model() {
    for (int i = 0; i < NUM_INPUTS; i++) {
        // scaler_scale[i] không nên = 0, nhưng cộng 1e-8 cho chắc
        input_buffer[i] = (segment[i] - scaler_mean[i]) / (scaler_scale[i] + 1e-8f);
    }
}

void setup() {
    analogReadResolution(ADC_BITS);
    pinMode(ADC_PIN, INPUT);
    pinMode(LO_POS, INPUT);
    pinMode(LO_NEG, INPUT);
    pinMode(GRN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);

    digitalWrite(GRN_LED, HIGH);
    digitalWrite(RED_LED, HIGH);

    // ===== timer configuration: 8ms = 8000 µs → 125 Hz =====
    const esp_timer_create_args_t timer_args = {
        .callback = &onTimer,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ECG_Sampler"
    };

    esp_timer_handle_t periodic_timer;
    esp_timer_create(&timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 8000);

    // ===== INIT TINYML / MODEL =====
    // cấu hình shape I/O
    tf.setNumInputs(NUM_INPUTS);
    tf.setNumOutputs(NUM_OUTPUTS);

    // đăng ký các ops mà model 1D-CNN cần
    tf.resolver.AddConv2D();         // Conv1D -> Conv2D
    tf.resolver.AddMaxPool2D();      // MaxPooling1D -> MaxPool2D
    tf.resolver.AddFullyConnected(); // Dense
    tf.resolver.AddRelu();           // activation='relu'
    tf.resolver.AddSoftmax();        // lớp cuối softmax

    // xử lý shape nội bộ (Flatten, ExpandDims, v.v.)
    tf.resolver.AddReshape();
    tf.resolver.AddExpandDims();

    // BatchNorm gộp thành toán tử số học
    tf.resolver.AddMul();            // nhân hệ số BN
    tf.resolver.AddAdd();            // cộng bias BN

    // nạp model
    auto result = tf.begin(g_model);
    if (!result.isOk()) {
        while (true) {
            digitalWrite(GRN_LED, LOW);
            delay(500);
            digitalWrite(GRN_LED, HIGH);
            delay(500);
        }     // đứng im nếu lỗi model
    }

    digitalWrite(GRN_LED, LOW);
    digitalWrite(RED_LED, LOW);
}

void loop() {
    if ((digitalRead(LO_POS) == HIGH) || (digitalRead(LO_NEG) == HIGH)) {
        while(digitalRead(LO_POS) == HIGH || digitalRead(LO_NEG) == HIGH) {
            digitalWrite(GRN_LED, HIGH);
            digitalWrite(RED_LED, HIGH);
            delay(100);
            digitalWrite(GRN_LED, LOW);
            digitalWrite(RED_LED, LOW);
            delay(100);
        }
        digitalWrite(GRN_LED, LOW);
        digitalWrite(RED_LED, LOW);
    } else {
        // ===== Start Acquiring =====
        for (int i = 0; i < 3; i++) {
            digitalWrite(GRN_LED, HIGH);
            delay(100);
            digitalWrite(GRN_LED, LOW);
            delay(100);
        }
        acquire_window();
        
        // ===== DETECT TWO PEAKS =====
        // Z-score normalization cho R-peak detection
        ecg_mean  = mean(ecg_raw, samples);
        ecg_std   = npstd(ecg_raw, samples);
        for (int i = 0; i < samples; i++) {
            ecg_norm[i] = (ecg_raw[i] - ecg_mean) / (ecg_std + 1e-8f);
        }

        // peaks
        uint8_t peak_count = find_two_peaks(ecg_norm, samples, 3.0f, 60, peaks);
        
        if (peak_count < 2) {
            for (int i = 0; i < 3; i++) {
                digitalWrite(GRN_LED, HIGH);
                digitalWrite(RED_LED, HIGH);
                delay(100);
                digitalWrite(GRN_LED, LOW);
                digitalWrite(RED_LED, LOW);
                delay(100);
            }
            return;
        }

        // ===== CẮT SEGMENT THEO 2 ĐỈNH R =====
        start = peaks[0];
        end = start + int(1.2f * (peaks[1] - start));

        // bảo vệ không vượt khỏi ecg_raw
        if (end >= samples - 1) {
            for (int i = 0; i < 3; i++) {
                digitalWrite(GRN_LED, HIGH);
                digitalWrite(RED_LED, HIGH);
                delay(100);
                digitalWrite(GRN_LED, LOW);
                digitalWrite(RED_LED, LOW);
                delay(100);
            }
            return;
        }

        uint16_t seg_len = end - start + 1;

        // bảo vệ không vượt NUM_INPUTS (187)
        if (seg_len > NUM_INPUTS)   
            seg_len = NUM_INPUTS;
        
        for (int i = 0; i < seg_len; i++){
            segment[i] = ecg_raw[start + i];
        }

        // ===== MIN-MAX TRÊN CHÍNH SEGMENT =====
        seg_min = array_min(segment, seg_len);
        seg_max = array_max(segment, seg_len);

        if (seg_max > seg_min) {
            for (int i = 0; i < seg_len; i++) {
                segment[i] = (segment[i] - seg_min)/(seg_max - seg_min);
            }
        }

        // pad 0 cho đủ 187
        for (int i = seg_len; i < NUM_INPUTS; i++) {
            segment[i] = 0.0f;
        }

        // ===== CHUẨN HOÁ VÀ CHẠY MODEL =====
        normalize_segment_for_model();   // fill input_buffer[0..186]

        auto inferResult = tf.predict(input_buffer);
        if (!inferResult.isOk()) {
            digitalWrite(GRN_LED, HIGH);
            digitalWrite(RED_LED, HIGH);
            for (int i = 0; i < 5; i++) {
                delay(100);
                digitalWrite(RED_LED, LOW);
                delay(100);
                digitalWrite(RED_LED, HIGH);
            }
            digitalWrite(GRN_LED, LOW);
            digitalWrite(RED_LED, LOW);
            return;
        }

        float z0 = tf.output(0);   // Normal
        float z1 = tf.output(1);   // Abnormal

        int label = (z1 > z0) ? 1 : 0;

        if (label == 0) {
            digitalWrite(GRN_LED, HIGH);
            digitalWrite(RED_LED, LOW);
        } else if (label == 1) {
            digitalWrite(GRN_LED, LOW);
            digitalWrite(RED_LED, HIGH);
        }
        delay(1000);
        digitalWrite(GRN_LED, LOW);
        digitalWrite(RED_LED, LOW);
    }
}
