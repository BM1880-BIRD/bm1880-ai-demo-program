#include "bmnet_layer.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>

static bmnet_t bm_net;
static bmctx_t bm_ctx;
bool blInit = false;

bmnet_layer make_bmnet_layer(int batch, int h, int w, int c, int out_h,
                             int out_w, int out_c, char *bmodel_file)
{
  bmnet_layer l = {0};

  l.type = BMNET;
  l.batch = batch;
  l.h = h;
  l.w = w;
  l.c = c;
  l.out_h = out_h;
  l.out_w = out_w;
  l.out_c = out_c;
  l.outputs = out_h*out_w*out_c;
  l.inputs = h*w*c;
  int output_size = l.outputs * batch;
  l.output =  calloc(output_size, sizeof(float));
  if (l.output == NULL) {
    fprintf(stderr, "memory alloc failed.\n");
    exit(-1);
  }
  l.delta = 0;
  l.forward = forward_bmnet_layer;
  l.loop_forward = loop_forward_bmnet_layer;
  l.backward = backward_bmnet_layer;
  l.bmodel_file = calloc(strlen(bmodel_file)+1, sizeof(char));
  if (l.output == NULL) {
    fprintf(stderr, "memory alloc failed.\n");
    exit(-1);
  }
  strcpy(l.bmodel_file, bmodel_file);

  return l;
}

void int8_quantization(int8_t *out, const float *in, float threshold, int length) {
  for (int i = 0; i < length; i++) {
    float data = in[i];
    int fixed_data = (int)(data * (128.0 / threshold) + 0.5);
    out[i] = (fixed_data < -128) ? -128 : ((fixed_data > 127) ? 127 : fixed_data);
  }
}

void int8_dequantization(float *out, const int8_t *in, float threshold, int length) {
  for (int i = 0; i < length; i++) {
    int data = (int)in[i];
    out[i] = data * (threshold / 128.0);
  }
}

void init_bmnet(const bmnet_layer l) {
  double time_temp;
  time_temp = what_time_is_it_now();
  bmerr_t ret;
  ret = bm_init(0, &bm_ctx);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "bm_init failed, err %d\n", ret);
    exit(-1);
  }
  printf("bm_init time:%f\n", what_time_is_it_now() - time_temp);

  time_temp = what_time_is_it_now();
  ret = bmnet_register_bmodel(bm_ctx, l.bmodel_file, &bm_net);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "register failed, err %d\n", ret);
    bm_exit(bm_ctx);
    exit(-1);
  }
  printf("bmnet_register:%f\n", what_time_is_it_now() - time_temp);

  blInit = true;
}

void destroy_bmnet() {
  printf("destory_bmnet\n");
  bmnet_cleanup(bm_net);
  bm_exit(bm_ctx);
  blInit = false;
}


void loop_forward_bmnet_layer(const bmnet_layer l, network net)
{
  bmerr_t ret;
  bmnet_output_info_t output_info;
  shape_t input_shape = shape_t4(l.batch, l.c, l.h, l.w);

  int input_length = l.inputs * l.batch;
  int output_length = l.outputs * l.batch;


  uint8_t *int8_input = calloc(input_length, sizeof(char));
  if (int8_input == NULL) {
    fprintf(stderr, "input memory alloc failed.\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  int8_quantization((int8_t *)int8_input, net.input,
      bmmet_get_input_threshold(bm_net), input_length);

#if 0
  //input fp32 file
  FILE *fp = fopen("input_fp32.bin","wb");
  fwrite(net.input,1,input_length*4,fp);
  fclose(fp);

  //input int8 file
  FILE *fp1 = fopen("input_int8.bin","wb");
  fwrite(int8_input,1,input_length,fp1);
  fclose(fp1);
#endif

  ret = bmnet_set_input_shape(bm_net, input_shape);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "input_shape not suppport\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  ret = bmnet_get_output_info(bm_net, &output_info);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "get output failed\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  size_t output_size = output_info.output_size;
  uint8_t *int8_output = (uint8_t *)calloc(output_size, sizeof(char));
  if (int8_output == NULL) {
    fprintf(stderr, "output memory alloc failed.\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  ret = bmnet_inference(bm_net, int8_input, int8_output);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "inference failed.\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
  }

#if 0
  //output int8 file
  FILE *fp2 = fopen("output_int8.bin","wb");
  fwrite(int8_output,1,output_length,fp2);
  fclose(fp2);
#endif

  float *fp32_output_idx = l.output;
  char *int8_output_idx = int8_output;
  for (int i = 0; i < output_info.output_num; i++) {
    int sub_output_size = output_info.shape_array[i].n*output_info.shape_array[i].c*output_info.shape_array[i].h*output_info.shape_array[i].w;
    //printf("num = %d,length=%d,threadhold = %f \r\n",i,sub_output_size,output_info.threshold_array[i]);
    int8_dequantization(fp32_output_idx, (int8_t *)int8_output_idx, output_info.threshold_array[0], sub_output_size);
    fp32_output_idx += sub_output_size;
    int8_output_idx += sub_output_size;
  }

  free(int8_input);
  free(int8_output);
}

void forward_bmnet_layer(const bmnet_layer l, network net)
{
  if (false == blInit) {
    init_bmnet(l);
  }
  bmerr_t ret;
  bmnet_output_info_t output_info;
  shape_t input_shape = shape_t4(l.batch, l.c, l.h, l.w);

  int input_length = l.inputs * l.batch;
  int output_length = l.outputs * l.batch;

  uint8_t *int8_input = calloc(input_length, sizeof(char));
  if (int8_input == NULL) {
    fprintf(stderr, "input memory alloc failed.\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  int8_quantization((int8_t *)int8_input, net.input,
      bmmet_get_input_threshold(bm_net), input_length);

#if 0
  //input fp32 file
  FILE *fp = fopen("input_fp32.bin","wb");
  fwrite(net.input,1,input_length*4,fp);
  fclose(fp);

  //input int8 file
  FILE *fp1 = fopen("input_int8.bin","wb");
  fwrite(int8_input,1,input_length,fp1);
  fclose(fp1);
#endif

  ret = bmnet_set_input_shape(bm_net, input_shape);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "input_shape not suppport\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  ret = bmnet_get_output_info(bm_net, &output_info);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "get output failed\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  size_t output_size = output_info.output_size;
  uint8_t *int8_output = (uint8_t *)calloc(output_size, sizeof(char));
  if (int8_output == NULL) {
    fprintf(stderr, "output memory alloc failed.\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
    exit(-1);
  }

  ret = bmnet_inference(bm_net, int8_input, int8_output);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "inference failed.\n");
    bmnet_cleanup(bm_net);
    bm_exit(bm_ctx);
  }

#if 0
  //output int8 file
  FILE *fp2 = fopen("output_int8.bin","wb");
  fwrite(int8_output,1,output_length,fp2);
  fclose(fp2);
#endif

  float *fp32_output_idx = l.output;
  char *int8_output_idx = int8_output;
  for (int i = 0; i < output_info.output_num; i++) {
    int sub_output_size = output_info.shape_array[i].n*output_info.shape_array[i].c*output_info.shape_array[i].h*output_info.shape_array[i].w;
    //printf("num = %d,length=%d,threadhold = %f \r\n",i,sub_output_size,output_info.threshold_array[i]);
    int8_dequantization(fp32_output_idx, (int8_t *)int8_output_idx, output_info.threshold_array[0], sub_output_size);
    fp32_output_idx += sub_output_size;
    int8_output_idx += sub_output_size;
  }

  free(int8_input);
  free(int8_output);
}

void backward_bmnet_layer(const bmnet_layer l, network net)
{
    printf("bmnet does not support training\n");
    assert(0);
}

