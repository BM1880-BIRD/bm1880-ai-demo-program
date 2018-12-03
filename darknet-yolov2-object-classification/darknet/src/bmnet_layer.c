#include "bmnet_layer.h"
#include "utils.h"
#include <stdio.h>
#include <assert.h>

static bmnet_t bm_net;
static bmctx_t bm_ctx;
bool blInit = false;

bmnet_layer make_bmnet_layer(int batch, int h, int w, int c, int out_h,
                             int out_w, int out_c,
                             unsigned long long output_offset,
                             unsigned long long neuron_size,
                             char *weight_file, char *cmdbuf_file,
                             int is_int8, float in_threshold) {
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
    l.delta = 0;
    l.forward = forward_bmnet_layer;
    l.loop_forward = loop_forward_bmnet_layer;
    l.backward = backward_bmnet_layer;
    l.in_threshold = in_threshold;
    l.is_int8 = is_int8;

    FILE *fp;
    fp = fopen(weight_file, "rb");
    fseek(fp, 0, SEEK_END);
    int weight_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    l.weight = calloc(weight_size, sizeof(char));
    fread(l.weight, 1, weight_size, fp);
    close(fp);
    fp = fopen(cmdbuf_file, "rb");
    fseek(fp, 0, SEEK_END);
    int cmdbuf_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    l.cmdbuf = calloc(cmdbuf_size, sizeof(char));
    fread(l.cmdbuf, 1, cmdbuf_size, fp);
    fclose(fp);

    int unit_size = 4;
    if (is_int8 == 1) {
      unit_size = 1;
    }
    bmnet_info_t info = {
      .weight = l.weight,
      .weight_size = weight_size,
      .cmdbuf = l.cmdbuf,
      .cmdbuf_size = cmdbuf_size,
      .batch_size = batch,
      .input_size = l.inputs * l.batch * unit_size,
      .output_offset = output_offset,
      .output_size = l.outputs * l.batch * unit_size,
      .neuron_size = neuron_size
    };
    l.info = info;

    fprintf(stderr, "bmnet                                %4d x%4d x%4d  ->  %4d x%4d x%4d\n", w, h, c, out_h, out_w, out_c);
    return l;
}

void int8_quantization(char *out, const float *in, float threshold, int length) {
  for (int i = 0; i < length; i++) {
    float data = in[i];
    int fixed_data = (int)(data * (128.0 / threshold) + 0.5);
    out[i] = (fixed_data < -128) ? -128 : ((fixed_data > 127) ? 127 : fixed_data);
  }
}

void int8_dequantization(float *out, const char *in, float threshold, int length) {
  for (int i = 0; i < length; i++) {
    int data = (int)in[i];
    out[i] = data * (threshold / 128.0);
  }
}

void init_bmnet(const bmnet_layer l) {
  double time_temp;
  time_temp = what_time_is_it_now();
  bm_init(0, &bm_ctx);
  printf("bm_init time:%f\n", what_time_is_it_now() - time_temp);

  time_temp = what_time_is_it_now();
  bmnet_register(bm_ctx, &l.info, &bm_net);
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
    //BMNET_LOG("bmnet layer, nchw = %d,%d,%d,%d, out chw = %d,%d,%d\n",
    //    l.batch, l.c, l.h, l.w, l.out_c, l.out_h, l.out_w);
    //BMNET_DUMP("bmnet_input", net.input, l.batch, l.c, l.h, l.w);

    if (l.is_int8 == 1) {
      assert(l.in_threshold != 0);
      assert(l.out_num != 0);

      int input_length = l.inputs * l.batch;
      int output_length = l.outputs * l.batch;
      char *int8_input = calloc(input_length, sizeof(char));
      char *int8_output = calloc(output_length, sizeof(char));

      int8_quantization(int8_input, net.input, l.in_threshold, input_length);

      bmnet_inference(bm_net, int8_input, int8_output);
      float *fp32_output_idx = l.output;
      char *int8_output_idx = int8_output;
      for (int i = 0; i < l.out_num; i++) {
        int8_dequantization(fp32_output_idx, int8_output_idx, l.sub_output_threshold[i], l.sub_output_size[i]);
        fp32_output_idx += l.sub_output_size[i];
        int8_output_idx += l.sub_output_size[i];
      }

      free(int8_input);
      free(int8_output);
    } else {
      bmnet_inference(bm_net, net.input, l.output);
    }
}

void forward_bmnet_layer(const bmnet_layer l, network net)
{
  // BMNET_LOG("bmnet layer, nchw = %d,%d,%d,%d, out chw = %d,%d,%d\n",
    //    l.batch, l.c, l.h, l.w, l.out_c, l.out_h, l.out_w);
    // BMNET_DUMP("bmnet_input", net.input, l.batch, l.c, l.h, l.w);
    // bmnet_t bm_net;
    // bmctx_t bm_ctx;
    // bm_init(0, &bm_ctx);
    // bmnet_register(bm_ctx, &l.info, &bm_net);

  if (false == blInit) {
    init_bmnet(l);
  }

  if (l.is_int8 == 1) {
    assert(l.in_threshold != 0);
    assert(l.out_num != 0);

    int input_length = l.inputs * l.batch;
    int output_length = l.outputs * l.batch;
    char *int8_input = calloc(input_length, sizeof(char));
    char *int8_output = calloc(output_length, sizeof(char));

    int8_quantization(int8_input, net.input, l.in_threshold, input_length);

    bmnet_inference(bm_net, int8_input, int8_output);
    float *fp32_output_idx = l.output;
    char *int8_output_idx = int8_output;
    for (int i = 0; i < l.out_num; i++) {
      int8_dequantization(fp32_output_idx, int8_output_idx, l.sub_output_threshold[i], l.sub_output_size[i]);
      fp32_output_idx += l.sub_output_size[i];
      int8_output_idx += l.sub_output_size[i];
    }

    free(int8_input);
    free(int8_output);
  } else {
    bmnet_inference(bm_net, net.input, l.output);
  }

  //BMNET_DUMP("bmnet_output", l.output, l.batch, 1, 1, l.outputs);
  // bmnet_cleanup(bm_net);
  // bm_exit(bm_ctx);
}

void backward_bmnet_layer(const bmnet_layer l, network net)
{
    printf("bmnet does not support training\n");
    assert(0);
}

