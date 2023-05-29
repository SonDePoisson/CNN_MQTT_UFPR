#include <Arduino.h>
#include <math.h>
// WiFi
#include "WiFi.h"
// I2S
#include "I2S_define.h"
// TensorFlow
#include "TensorFlow_define.h"

void setup() 
{
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n\nStarting setup...");

  // start up the I2S peripheral
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
	Serial.println("I2S driver installed!");

  // Load the sample sine model
	all_target_model = tflite::GetModel(___builded_files_all_targets_tflite);
	Serial.println("TensorFlow model loaded!");

  // Define ops resolver and error reporting
	static tflite::ops::micro::AllOpsResolver resolver;
	static tflite::ErrorReporter* error_reporter;
	static tflite::MicroErrorReporter micro_error;
	error_reporter = &micro_error;

	// Instantiate the interpreter 
	static tflite::MicroInterpreter static_interpreter(
		all_target_model, resolver, tensor_pool, tensor_pool_size, error_reporter
	);

	interpreter = &static_interpreter;

	// Allocate the the model's tensors in the memory pool that was created.
	if(interpreter->AllocateTensors() != kTfLiteOk) {
		Serial.println("There was an error allocating the memory...ooof");
		return;
	}
	Serial.println("Memory allocation successful!");

	// Define input and output nodes
	input = interpreter->input(0);
	output = interpreter->output(0);

  Serial.println("Setup Complete");

	Serial.println("\nSpeak into the microphone to get a prediction\n");
}

void loop() 
{
  // read from the I2S device
  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int32_t);
  
  if (samples_read != SAMPLE_BUFFER_SIZE)
  {
    Serial.printf("Read %d samples from I2S device\n", samples_read);
  }
  else
  {
    // Set the input node to the user input
    for (int i = 0; i < samples_read; i++)
    {
      input->data.f[i] = raw_samples[i];
      // Serial.printf("Input(%d): %d\n", i, input->data.f[i]);
    }

    // Run inference on the input data
    if(interpreter->Invoke() != kTfLiteOk) {
      Serial.println("There was an error invoking the interpreter!");
      return;
    }

    // Print the output of the model.
    for (size_t i = 0; i < LABELS_COUNT; i++)
    {
      if (output->data.f[i] > 0.7)
        Serial.print("=> ");
      Serial.printf("%-9s: %9.3f\n", labels[i], output->data.f[i]);
    }

    Serial.printf("\n\n");

    delay(3000);
  }
}
