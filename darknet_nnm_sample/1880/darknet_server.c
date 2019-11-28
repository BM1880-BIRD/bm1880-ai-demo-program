#include <stdio.h>
#include <unistd.h>
#include "darknet.h"
#include "bmnet_layer.h"
#include "bm1880_usb.h"

extern unsigned char *stbi_load_from_memory(unsigned char const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);

image load_image_from_memory(unsigned char *buffer, int length)
{
    int channels = 3;
    int w, h, c;
    unsigned char *data = stbi_load_from_memory(buffer, length, &w, &h, &c, channels);
    if (!data) {
        printf("Cannot load image.\n");
        exit(0);
    }
    if(channels) c = channels;
    int i,j,k;
    image im = make_image(w, h, c);
    for(k = 0; k < c; ++k){
        for(j = 0; j < h; ++j){
            for(i = 0; i < w; ++i){
                int dst_index = i + w*j + w*h*k;
                int src_index = k + c*i + c*w*j;
                im.data[dst_index] = (float)data[src_index]/255.;
            }
        }
    }
    free(data);
    return im;
}

void write_png_to_memory(image im, unsigned char *outbuffer, int *outlength)
{
    unsigned char *png = 0;
    unsigned char *data = calloc(im.w*im.h*im.c, sizeof(char));
    int i,k;
    for(k = 0; k < im.c; ++k){
        for(i = 0; i < im.w*im.h; ++i){
            data[i*im.c+k] = (unsigned char) (255*im.data[i + k*im.w*im.h]);
        }
    }
    png = stbi_write_png_to_mem(data, im.w*im.c, im.w, im.h, im.c, outlength);
    printf("png size = %d\n", *outlength);
    memcpy(outbuffer, png, *outlength);
    free(png);
    free(data);

    return;
}

void yolo_inference(char *datacfg, char *cfgfile, char *weightfile, char *namefile, unsigned char *buffer, int length, float thresh, float hier_thresh, unsigned char *outbuffer, int *outlength)
{
    list *options = read_data_cfg(datacfg);
    char *name_list = option_find_str(options, "names", namefile);
    char **names = get_labels(name_list);

    image **alphabet = load_alphabet();
    network *net = load_network(cfgfile, weightfile, 0);
    set_batch_network(net, 1);
    srand(2222222);
    double time;
    float nms=.45;

    layer l = net->layers[net->n-1];
    //init
    if (BMNET == l.type) {
        init_bmnet(l);
    }

    image im = load_image_from_memory(buffer, length);
    image sized = letterbox_image(im, net->w, net->h);

    float *X = sized.data;
    time=what_time_is_it_now();
    network_predict(net, X);
    printf("Predicted in %f seconds.\n", what_time_is_it_now()-time);
    int nboxes = 0;
    detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
    if (nms) do_nms_sort(dets, nboxes, l.classes, nms);
    draw_detections(im, dets, nboxes, thresh, names, alphabet, l.classes);
    free_detections(dets, nboxes);

    write_png_to_memory(im, outbuffer, outlength);

    free_image(im);
    free_image(sized);

    if (BMNET == l.type) {
        destroy_bmnet();
    }

    return;
}

int main(int argc, char **argv)
{
    const int BUFFER_SIZE = 1024*1024;

    float thresh = find_float_arg(argc, argv, "-thresh", .5);

    unsigned char buffer[BUFFER_SIZE];
    int length = 0;

    unsigned char outbuffer[BUFFER_SIZE];
    unsigned char *tmpbuf =  outbuffer;
    int outlength;
    int remainlength;
    int writelength;
    int split = 60*1024;

    if (-1 == bm1880_usb_open())
    {
        printf("usb open fail\n");
        return -1;
    }

    while (1)
    {
        memset(buffer , 0 , BUFFER_SIZE);
        memset(outbuffer , 0 , BUFFER_SIZE);

        // get data from host
        printf("waiting data..\n");
        while (1)
        {
            length = bm1880_usb_read(buffer, BUFFER_SIZE, 1 * 1000);
            if (length > 0)
            {
                break;
            }
            usleep(10* 1000);
        }
        printf("read %d bytes data\n", length);

        // inference
        printf("inference..\n");
        yolo_inference(argv[1], argv[2], argv[3], argv[4], buffer, length, thresh, .5, outbuffer, &outlength);
        printf("inference done\n");

        // send result to host
        // write size can not bigger than 64KB
        printf("image size is %d bytes\n", outlength);
        printf("writing data..\n");
        remainlength = outlength;
        while (remainlength > 0)
        {
            writelength = remainlength > split ? split : remainlength;
            length = bm1880_usb_write(tmpbuf, writelength, 1 * 1000);
            printf("write %d bytes data\n", length);
            tmpbuf += split;
            remainlength -= split;
        }
        // send finish signal
        usleep(1000);
        length = bm1880_usb_write("NoMoreData", 11, 1 * 1000);
        printf("writing data done\n");
    }

    bm1880_usb_close();

    return 0;
}

