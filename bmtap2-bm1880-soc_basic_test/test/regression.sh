#!/bin/bash
#set -euo pipefail

BMNET_MODELS_PATH=${BMNET_MODELS_PATH:-/data/release/bmnet_models/}
BMNET_OUT_PATH=${BMNET_OUT_PATH:-out/}
mkdir -p ${BMNET_OUT_PATH}

echo "BMNET_MODELS_PATH = $BMNET_MODELS_PATH"
echo "BMNET_OUT_PATH = $BMNET_OUT_PATH"
echo "Usage:"
echo "      run_regression_bmnet_bm1880"

function print_error_bmnet()
{
  echo ${FUNCNAME[1]}: $* >&2
  return -1
}

function bmodel_test_bm1880()
{
  epsilon=0

  echo "begin to test net: $1 $2 <$3,$4,$5,$6>"

  path=$1
  net=$2
  batch_num=$3

  have_caffemodel="-c $BMNET_MODELS_PATH/${path}/${7}"
  have_modified_proto=""
  if [ $# == 9 ]; then
    have_modified_proto="-m ${BMNET_MODELS_PATH}/${path}/${9}"
  fi
  have_weight_bin=""

  (
      pushd $BMNET_OUT_PATH &&
          bm_builder.bin \
          -t bm1880 \
          -n ${net} \
          $have_caffemodel \
          $have_modified_proto \
          $have_weight_bin \
          --in_ctable=$BMNET_MODELS_PATH/${path}/${8} \
          --out_ctable=${net}_ctable_opt.pb2 \
          --enable-weight-optimize=yes \
          -u /usr/lib \
          -s $3,$4,$5,$6 \
          -p ${net}_frontend_opt.proto \
          -o ${net}_$3_$4_$5_$6.bmodel &&
      test_bmnet_bmodel \
          $BMNET_MODELS_PATH/${path}/${net}_input_$3_$4_$5_$6.bin \
          ${net}_$3_$4_$5_$6.bmodel \
          ${net}_output_$3_$4_$5_$6_int8.bin \
          $3 $4 $5 $6 &&
      bintool compare int8 \
          $BMNET_MODELS_PATH/${path}/${net}_output_$3_$4_$5_$6_ref_int8.bin \
          ${net}_output_$3_$4_$5_$6_int8.bin ${epsilon} &&
      popd
  ) || print_error_bmnet "${net}<$3,$4,$5,$6> failed" $? || return $?

  echo " ${net}<$3,$4,$5,$6> passed"
}

function run_regression_bmnet_bm1880()
{
  bmodel_test_bm1880 alexnet alexnet 1 3 227 227 bmnet_alexnet_int8.1x10.caffemodel bmnet_alexnet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 alexnet alexnet 2 3 227 227 bmnet_alexnet_int8.1x10.caffemodel bmnet_alexnet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 alexnet alexnet 4 3 227 227 bmnet_alexnet_int8.1x10.caffemodel bmnet_alexnet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 alexnet alexnet 8 3 227 227 bmnet_alexnet_int8.1x10.caffemodel bmnet_alexnet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 alexnet alexnet 16 3 227 227 bmnet_alexnet_int8.1x10.caffemodel bmnet_alexnet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 alexnet alexnet 32 3 227 227 bmnet_alexnet_int8.1x10.caffemodel bmnet_alexnet_calibration_table.1x10.pb2 \
            || return $?
#------------------------------resnet50-------------------------------

  bmodel_test_bm1880 resnet50 resnet50 1 3 224 224 bmnet_resnet50_int8.1x10.caffemodel bmnet_resnet50_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 resnet50 resnet50 2 3 224 224 bmnet_resnet50_int8.1x10.caffemodel bmnet_resnet50_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 resnet50 resnet50 4 3 224 224 bmnet_resnet50_int8.1x10.caffemodel bmnet_resnet50_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 resnet50 resnet50 8 3 224 224 bmnet_resnet50_int8.1x10.caffemodel bmnet_resnet50_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 resnet50 resnet50 16 3 224 224 bmnet_resnet50_int8.1x10.caffemodel bmnet_resnet50_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 resnet50 resnet50 32 3 224 224 bmnet_resnet50_int8.1x10.caffemodel bmnet_resnet50_calibration_table.1x10.pb2 \
            || return $?
#------------------------------googlenet-------------------------------

  bmodel_test_bm1880 googlenet googlenet 1 3 224 224 bmnet_googlenet_int8.1x10.caffemodel bmnet_googlenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 googlenet googlenet 2 3 224 224 bmnet_googlenet_int8.1x10.caffemodel bmnet_googlenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 googlenet googlenet 4 3 224 224 bmnet_googlenet_int8.1x10.caffemodel bmnet_googlenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 googlenet googlenet 8 3 224 224 bmnet_googlenet_int8.1x10.caffemodel bmnet_googlenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 googlenet googlenet 16 3 224 224 bmnet_googlenet_int8.1x10.caffemodel bmnet_googlenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 googlenet googlenet 32 3 224 224 bmnet_googlenet_int8.1x10.caffemodel bmnet_googlenet_calibration_table.1x10.pb2 \
            || return $?
#------------------------------mobilenet-------------------------------

  bmodel_test_bm1880 mobilenet mobilenet 1 3 224 224 bmnet_mobilenet_int8.1x10.caffemodel bmnet_mobilenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 mobilenet mobilenet 2 3 224 224 bmnet_mobilenet_int8.1x10.caffemodel bmnet_mobilenet_calibration_table.1x10.pb2 \
           || return $?
  bmodel_test_bm1880 mobilenet mobilenet 4 3 224 224 bmnet_mobilenet_int8.1x10.caffemodel bmnet_mobilenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 mobilenet mobilenet 8 3 224 224 bmnet_mobilenet_int8.1x10.caffemodel bmnet_mobilenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 mobilenet mobilenet 16 3 224 224 bmnet_mobilenet_int8.1x10.caffemodel bmnet_mobilenet_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 mobilenet mobilenet 32 3 224 224 bmnet_mobilenet_int8.1x10.caffemodel bmnet_mobilenet_calibration_table.1x10.pb2 \
            || return $?
#------------------------------vgg16-------------------------------

  bmodel_test_bm1880 vgg16 vgg16 1 3 224 224 bmnet_vgg16_int8.1x10.caffemodel bmnet_vgg16_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 vgg16 vgg16 2 3 224 224 bmnet_vgg16_int8.1x10.caffemodel bmnet_vgg16_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 vgg16 vgg16 4 3 224 224 bmnet_vgg16_int8.1x10.caffemodel bmnet_vgg16_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 vgg16 vgg16 8 3 224 224 bmnet_vgg16_int8.1x10.caffemodel bmnet_vgg16_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 vgg16 vgg16 16 3 224 224 bmnet_vgg16_int8.1x10.caffemodel bmnet_vgg16_calibration_table.1x10.pb2 \
            || return $?
  bmodel_test_bm1880 vgg16 vgg16 32 3 224 224 bmnet_vgg16_int8.1x10.caffemodel bmnet_vgg16_calibration_table.1x10.pb2 \
            || return $?
  echo ${FUNCNAME[0]} "passed"
}