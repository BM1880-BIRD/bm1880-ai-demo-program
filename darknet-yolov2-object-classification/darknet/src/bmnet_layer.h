#ifndef BMNET_LAYER_H
#define BMNET_LAYER_H

#include "image.h"
#include "layer.h"
#include "network.h"

typedef layer bmnet_layer;

bmnet_layer make_bmnet_layer(int batch, int h, int w, int c, int out_h,
                             int out_w, int out_c,
                             unsigned long long output_offset,
                             unsigned long long neuron_size,
                             char *weight_file, char *cmdbuf_file, int is_int8,
                             float in_threshold);
void forward_bmnet_layer(const bmnet_layer l, network net);
void backward_bmnet_layer(const bmnet_layer l, network net);
//test
void loop_forward_bmnet_layer(const bmnet_layer l, network net);
void init_bmnet(const bmnet_layer l);
void destory_bmnet();
#endif
