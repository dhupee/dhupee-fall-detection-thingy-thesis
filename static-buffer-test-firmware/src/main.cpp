#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <dh-thesis_inferencing.h>

static const float features[] = {
    2.6987,  2.6849,  8.2658,  0.0052,  -0.1243, -0.0283, 2.3053,  3.2010,
    9.6883,  0.0672,  -0.1618, -0.0805, 2.8871,  3.3641,  9.0966,  0.0986,
    -0.2075, -0.1156, 2.7430,  2.8997,  6.7728,  0.1322,  -0.2202, -0.1289,
    2.9363,  2.6105,  7.7092,  0.0960,  -0.3515, -0.1863, 2.6522,  2.4027,
    8.0696,  0.2012,  -0.4123, -0.2130, 3.4531,  2.7452,  8.5240,  0.2761,
    -0.6107, -0.2643, 3.1510,  3.4044,  7.4424,  -0.0624, -0.6043, -0.1459,
    3.4153,  4.4590,  7.6264,  -0.0977, -0.6477, -0.1927, 3.0465,  3.2873,
    8.8732,  -0.4154, -0.6765, -0.2262, 3.5480,  2.2110,  7.9864,  -0.2015,
    -0.9385, -0.2063, 3.9383,  3.3937,  6.9712,  -0.7690, -0.6897, -0.1978,
    4.8470,  3.5813,  7.5402,  -0.7029, -0.7709, -0.5101, 5.1742,  2.5745,
    6.5175,  -1.0739, -0.5493, -0.3649, 5.8372,  1.6854,  6.9832,  -1.1303,
    -0.3157, -0.4517, 5.5278,  1.1960,  5.2304,  -0.8771, -0.0205, -0.3887,
    6.3973,  0.4250,  5.7268,  -0.4292, 0.2595,  -0.4108, 4.2687,  1.5002,
    6.4220,  0.1544,  0.4388,  -0.5702, 3.3873,  2.9786,  5.7648,  0.2770,
    0.6093,  -0.5311, 4.7024,  3.8596,  9.3858,  0.1591,  0.6876,  -0.5157,
    3.6884,  3.7779,  10.1495, 0.1446,  0.7624,  -0.2364, 2.8000,  3.6473,
    8.7030,  -0.0293, 0.5725,  -0.0926, 3.0410,  2.8375,  9.0103,  0.0830,
    0.3923,  -0.0563, 3.1798,  1.8317,  9.6910,  0.1571,  0.3629,  -0.0596,
    2.1030,  2.1460,  8.6433,  0.2930,  0.2623,  -0.1115, 2.7183,  2.8064,
    8.9198,  0.4712,  0.1744,  -0.0982, 3.7081,  3.4613,  8.3170,  0.2850,
    0.2230,  -0.1173, 3.1643,  4.4904,  10.3289, 0.2098,  0.2104,  -0.0921,
    2.9933,  4.1654,  8.3874,  0.4387,  0.2199,  0.1337};

/**
 * @brief      Copy raw feature data in out_ptr
 *             Function called by inference library
 *
 * @param[in]  offset   The offset
 * @param[in]  length   The length
 * @param      out_ptr  The out pointer
 *
 * @return     0
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

void print_inference_result(ei_impulse_result_t result);

/**
 * @brief      Arduino setup function
 */
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // comment out the below line to cancel the wait for USB connection (needed
  // for native USB)
  while (!Serial)
    ;
  Serial.println("Edge Impulse Inferencing Demo");
}

/**
 * @brief      Arduino main function
 */
void loop() {
  ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

  if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
    ei_printf("The size of your 'features' array is not correct. Expected %lu "
              "items, but had %lu\n",
              EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
              sizeof(features) / sizeof(float));
    delay(1000);
    return;
  }

  ei_impulse_result_t result = {0};

  // the features are stored into flash, and we don't want to load everything
  // into RAM
  signal_t features_signal;
  features_signal.total_length = sizeof(features) / sizeof(features[0]);
  features_signal.get_data = &raw_feature_get_data;

  // invoke the impulse
  EI_IMPULSE_ERROR res =
      run_classifier(&features_signal, &result, false /* debug */);
  if (res != EI_IMPULSE_OK) {
    ei_printf("ERR: Failed to run classifier (%d)\n", res);
    return;
  }

  // print inference return code
  ei_printf("run_classifier returned: %d\r\n", res);
  print_inference_result(result);

  delay(1000);
}

void print_inference_result(ei_impulse_result_t result) {

  // Print how long it took to perform inference
  ei_printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
            result.timing.dsp, result.timing.classification,
            result.timing.anomaly);

  // Print the prediction results (object detection)
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
  ei_printf("Object detection bounding boxes:\r\n");
  for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
    ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
    if (bb.value == 0) {
      continue;
    }
    ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n", bb.label,
              bb.value, bb.x, bb.y, bb.width, bb.height);
  }

  // Print the prediction results (classification)
#else
  ei_printf("Predictions:\r\n");
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
    ei_printf("%.5f\r\n", result.classification[i].value);
  }
#endif

  // Print anomaly result (if it exists)
#if EI_CLASSIFIER_HAS_ANOMALY
  ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif

#if EI_CLASSIFIER_HAS_VISUAL_ANOMALY
  ei_printf("Visual anomalies:\r\n");
  for (uint32_t i = 0; i < result.visual_ad_count; i++) {
    ei_impulse_result_bounding_box_t bb = result.visual_ad_grid_cells[i];
    if (bb.value == 0) {
      continue;
    }
    ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n", bb.label,
              bb.value, bb.x, bb.y, bb.width, bb.height);
  }
#endif
}
