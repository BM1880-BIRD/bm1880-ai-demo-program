/*
 * Copyright Bitmain Technologies Inc.
 *
 * This sample code about how to use bmodel to do inference.
 * Exception code has been removed to show the main program procedure.
 * In product code, you need to judge exceptions, such as file not exist,
 * memory malloc failed, and so on.
 * Created Time: 2018-10-20 15:34
 */

#include <fstream>
#include <string>
#include <cstdio>
#include <bmkernel/bm_kernel.h>
#include <libbmruntime/bmruntime.h>
#include <libbmruntime/bmruntime_bmnet.h>

#define TEST_INPUT  "../../models/mtcnn/det1_input_1_3_12_12.bin"
#define TEST_BMODEL "../../models/mtcnn/det1_1_3_12_12.bmodel"

// show output information
static void print_output_info(bmnet_output_info_t *output_info) {
  printf("output size:%lu\n", output_info->output_size);
  for (size_t i = 0; i < output_info->output_num; i++) {
    printf("outputs[%lu]: [%u,%u,%u,%u], \"%s\"\n", i,
           output_info->shape_array[i].n, output_info->shape_array[i].c,
           output_info->shape_array[i].h, output_info->shape_array[i].w,
           output_info->name_array[i]);
  }
}

int main(int argc, char *argv[]) {
  bmctx_t ctx;
  bmerr_t ret;
  // * 1. init environment.
  ret = bm_init(0, &ctx);
  if (ret != BM_SUCCESS) {
    printf("Error, bm_init failed.\n");
    return ret;
  }
  // * 2. register net by bmodel file, and get net handle
  // make sure bmodel file is exist.
  bmnet_t net;
  ret = bmnet_register_bmodel(ctx, TEST_BMODEL, &net);
  if (ret != BM_SUCCESS) {
    printf("Error, regesiter failed.\n");
    bm_exit(ctx);
    return ret;
  }

  // * 3. set input shape.
  // Make sure this shape in supported by the bmodel file.
  // If not ,it will failed.
  ret = bmnet_set_input_shape(net, shape_t4(1, 3, 12, 12));
  if (ret != BM_SUCCESS) {
    // before exit, the resources need clean.
    printf("Error, (1,3,12,12) is not supported by this bmodel file.\n");
    bmnet_cleanup(net);
    bm_exit(ctx);
    return ret;
  }

  // * 4. get output information.
  // include output total size, and output shapes.
  bmnet_output_info_t output_info;
  bmnet_get_output_info(net, &output_info);
  print_output_info(&output_info);

  // * 5. do inference
  // Alloc output buffer for inference
  size_t output_size = output_info.output_size;
  uint8_t *output = new uint8_t[output_size];

  // Read input data for inference. Make sure file is exist
  std::fstream f_input(TEST_INPUT,
                       std::ios::in | std::ios::binary);
  if (!f_input) {
    // before exit, the resources need clean.
    printf("input file is not exist\n");
    bmnet_cleanup(net);
    bm_exit(ctx);
    return ret;
  }
  f_input.seekg(0, f_input.end);
  size_t input_size = f_input.tellg();
  uint8_t *input = new uint8_t[input_size];
  f_input.seekg(0, f_input.beg);
  f_input.read((char *)input, input_size);
  f_input.close();

  // do inference
  bmnet_inference(net, input, output);

  // write output result to file, and release buffer
  std::fstream f_output("output.bin",
                        std::ios::out | std::ios::trunc | std::ios::binary);
  f_output.write((char *)output, output_size);
  f_output.close();

  delete[] output;
  delete[] input;

  // * 6. clear resoure used by net handle
  bmnet_cleanup(net);

  // * 7. exit bm environment
  bm_exit(ctx);

  printf("Successfully. Result has stored in outptu.bin.\n");

  return 0;
}
