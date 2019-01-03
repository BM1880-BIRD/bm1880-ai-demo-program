/*
 * Copyright Bitmain Technologies Inc.
 *
 * This sample code shows how to use kernel to do tensor calculate.
 * Created Time: 2018-07-21 15:34
 */
#include <bmkernel/bm_kernel.h>
#include <bmkernel/bm1880/bmkernel_1880.h>
#include <libbmruntime/bmruntime.h>
#include <libbmruntime/bmruntime_bmkernel.h>
#include <stdio.h>
#include <stdlib.h>

static void tl_add_ref(
    uint8_t *ref_high, uint8_t *ref_low,
    uint8_t *a_high, uint8_t *a_low,
    uint8_t *b_high, uint8_t *b_low,
    uint64_t size)
{
  for (uint64_t i = 0; i < size; i++) {
    int32_t ta = ((int8_t)a_high[i] << 8) + a_low[i];
    int32_t tb = ((int8_t)b_high[i] << 8) + b_low[i];
    int32_t res = ta + tb;
    if (res > 32767)
      res = 32767;
    else if (res < -32768)
      res = -32768;
    ref_high[i] = (res >> 8) & 0xff;
    ref_low[i] = res & 0xff;
  }
}

static void test_tl_add(bmctx_t ctx, int N, int C, int H, int W)
{
  bmk1880_context_t *bk_ctx;
  printf("test_tl_add (%d, %d, %d, %d) begin ...\n", N, C, H, W);

  shape_t shape = shape_t4(N, C, H, W);
  uint64_t size = N*C*H*W;

  uint8_t *a_high_data = (uint8_t *)malloc(size);
  uint8_t *a_low_data = (uint8_t *)malloc(size);
  uint8_t *b_high_data = (uint8_t *)malloc(size);
  uint8_t *b_low_data = (uint8_t *)malloc(size);
  uint8_t *r_high_data = (uint8_t *)malloc(size);
  uint8_t *r_low_data = (uint8_t *)malloc(size);
  uint8_t *ref_high_data = (uint8_t *)malloc(size);
  uint8_t *ref_low_data = (uint8_t *)malloc(size);

  for (size_t i = 0; i < size; i++) {
    a_high_data[i] = i / 10;
    a_low_data[i] = i;
    b_high_data[i] = (i + 250) / 20;
    b_low_data[i] = 100 - i;
  }

  tl_add_ref(ref_high_data, ref_low_data,
             a_high_data, a_low_data,
             b_high_data, b_low_data,
             size);

  // alloc device memory
  bmshape_t bms = BM_TENSOR_INT8(N, C, H, W);
  bmmem_device_t devmem_a_low = bmmem_device_alloc(ctx, &bms);
  bmmem_device_t devmem_a_high = bmmem_device_alloc(ctx, &bms);
  bmmem_device_t devmem_b_low = bmmem_device_alloc(ctx, &bms);
  bmmem_device_t devmem_b_high = bmmem_device_alloc(ctx, &bms);
  bmmem_device_t devmem_r_low = bmmem_device_alloc(ctx, &bms);
  bmmem_device_t devmem_r_high = bmmem_device_alloc(ctx, &bms);
  gaddr_t gaddr_a_low  = bmmem_device_addr(ctx, devmem_a_low);
  gaddr_t gaddr_a_high = bmmem_device_addr(ctx, devmem_a_high);
  gaddr_t gaddr_b_low  = bmmem_device_addr(ctx, devmem_b_low);
  gaddr_t gaddr_b_high = bmmem_device_addr(ctx, devmem_b_high);
  gaddr_t gaddr_r_low  = bmmem_device_addr(ctx, devmem_r_low);
  gaddr_t gaddr_r_high = bmmem_device_addr(ctx, devmem_r_high);

  // copy to device memory
  bmerr_t ret = bm_memcpy_s2d(ctx, devmem_a_low, a_low_data);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "bm_memcpy_s2d failed, err %d\n", ret);
    exit(-1);
  }
  ret = bm_memcpy_s2d(ctx, devmem_a_high, a_high_data);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "bm_memcpy_s2d failed, err %d\n", ret);
    exit(-1);
  }
  ret = bm_memcpy_s2d(ctx, devmem_b_low, b_low_data);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "bm_memcpy_s2d failed, err %d\n", ret);
    exit(-1);
  }
  ret = bm_memcpy_s2d(ctx, devmem_b_high, b_high_data);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "bm_memcpy_s2d failed, err %d\n", ret);
    exit(-1);
  }

  // * create bmkernel environment, and get kernel handle.
  bmruntime_bmkernel_create(ctx, (void**)&bk_ctx);

  // * edit  command
  tensor_lmem *tl_a_low = bmk1880_tl_alloc(bk_ctx, shape, FMT_I8, CTRL_AL);
  tensor_lmem *tl_a_high = bmk1880_tl_alloc(bk_ctx, shape, FMT_I8, CTRL_AL);
  tensor_lmem *tl_b_low = bmk1880_tl_alloc(bk_ctx, shape, FMT_I8, CTRL_AL);
  tensor_lmem *tl_b_high = bmk1880_tl_alloc(bk_ctx, shape, FMT_I8, CTRL_AL);
  tensor_lmem *tl_r_low = bmk1880_tl_alloc(bk_ctx, shape, FMT_I8, CTRL_AL);
  tensor_lmem *tl_r_high = bmk1880_tl_alloc(bk_ctx, shape, FMT_I8, CTRL_AL);
  bmk1880_gdma_load(bk_ctx, tl_a_low, gaddr_a_low, CTRL_NULL);
  bmk1880_gdma_load(bk_ctx, tl_a_high, gaddr_a_high, CTRL_NULL);
  bmk1880_gdma_load(bk_ctx, tl_b_low, gaddr_b_low, CTRL_NULL);
  bmk1880_gdma_load(bk_ctx, tl_b_high, gaddr_b_high, CTRL_NULL);

  bmk1880_add_param_t p4;
  p4.res_high = tl_r_high;
  p4.res_low = tl_r_low;
  p4.a_high = tl_a_high;
  p4.a_low = tl_a_low;
  p4.b_high = tl_b_high;
  p4.b_low = tl_b_low;
  bmk1880_tpu_add(bk_ctx, &p4);
  bmk1880_gdma_store(bk_ctx, tl_r_low, gaddr_r_low, CTRL_NULL);
  bmk1880_gdma_store(bk_ctx, tl_r_high, gaddr_r_high, CTRL_NULL);
  bmk1880_tl_free(bk_ctx, tl_r_high);
  bmk1880_tl_free(bk_ctx, tl_r_low);
  bmk1880_tl_free(bk_ctx, tl_b_high);
  bmk1880_tl_free(bk_ctx, tl_b_low);
  bmk1880_tl_free(bk_ctx, tl_a_high);
  bmk1880_tl_free(bk_ctx, tl_a_low);

  // * submit command
  bmruntime_bmkernel_submit(ctx);

  // * release bmkernel resources
  bmruntime_bmkernel_destroy(ctx);

  // * copy result from device memory to system memory
  ret = bm_memcpy_d2s(ctx, r_high_data, devmem_r_high);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "bm_memcpy_s2d failed, err %d\n", ret);
    exit(-1);
  }
  ret = bm_memcpy_d2s(ctx, r_low_data, devmem_r_low);
  if (ret != BM_SUCCESS) {
    fprintf(stderr, "bm_memcpy_s2d failed, err %d\n", ret);
    exit(-1);
  }

  // * free device memory
  bmmem_device_free(ctx, devmem_a_low);
  bmmem_device_free(ctx, devmem_a_high);
  bmmem_device_free(ctx, devmem_b_low);
  bmmem_device_free(ctx, devmem_b_high);
  bmmem_device_free(ctx, devmem_r_low);
  bmmem_device_free(ctx, devmem_r_high);

  // here compare result
  for (size_t i = 0; i < size; i++) {
    if (r_high_data[i] != ref_high_data[i]) {
      printf("comparing failed at res_high_data[%lu], got %u, exp %u\n",
             i, r_high_data[i], ref_high_data[i]);
      exit(-1);
    }
    if (r_low_data[i] != ref_low_data[i]) {
      printf("comparing failed at res_low_data[%lu], got %u, exp %u\n",
             i, r_low_data[i], ref_low_data[i]);
      exit(-1);
    }
  }

  free(a_high_data);
  free(a_low_data);
  free(b_high_data);
  free(b_low_data);
  free(ref_high_data);
  free(ref_low_data);
  free(r_high_data);
  free(r_low_data);

  printf("test_tl_add (%d, %d, %d, %d) success\n", N, C, H, W);
}

int main(int argc, char *argv[])
{
  // * init bm environment, and get bm handle.
  bmctx_t ctx;
  bmerr_t ret = bm_init(0, &ctx);
  if (ret != BM_SUCCESS) {
    printf("bm_init failed, err %d\n", ret);
    return ret;
  }

  test_tl_add(ctx, 1, 3, 12, 12);
  test_tl_add(ctx, 4, 3, 24, 24);

  // * exit bm environment
  bm_exit(ctx);


  return 0;
}
