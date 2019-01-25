#ifndef BMNET_LAYER_H
#define BMNET_LAYER_H

#include "image.h"
#include "layer.h"
#include "network.h"

typedef layer bmnet_layer;

bmnet_layer make_bmnet_layer(int batch, int h, int w, int c, int out_h,
                             int out_w, int out_c,
                             char *bmodel_file);
void forward_bmnet_layer(const bmnet_layer l, network net);
void backward_bmnet_layer(const bmnet_layer l, network net);
//test
void loop_forward_bmnet_layer(const bmnet_layer l, network net);
void init_bmnet(const bmnet_layer l);
void destroy_bmnet();
#endif
