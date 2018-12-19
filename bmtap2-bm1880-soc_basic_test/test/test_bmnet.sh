#!/bin/sh
#set -euo pipefail

SCRIPT_DIR=$(pwd)

BMNET_OUT_PATH=${BMNET_OUT_PATH:-out/}
mkdir -p ${BMNET_OUT_PATH}

echo "BMNET_OUT_PATH = $BMNET_OUT_PATH"

function print_error_bmnet()
{
  echo ${FUNCNAME[1]}: $* >&2
  return -1
}

function test_bmnet_bm1880()
{

  echo "begin to test net: $1 $2 <$3,$4,$5,$6>"

  path=${SCRIPT_DIR}/../models/$1
  net=$2
  batch_num=$3

  ../bin/test_bmnet_bmodel \
      ${path}/${net}_input_$3_$4_$5_$6.bin \
      ${path}/${net}_$3_$4_$5_$6.bmodel \
      ${BMNET_OUT_PATH}/${net}_output.bin \
      $3 $4 $5 $6 ||
  print_error_bmnet "test ${net}<$3,$4,$5,$6> failed" $? || return $?

  (
    ../bin/bintool compare int8 \
            ${path}/${net}_output_$3_$4_$5_$6_ref.bin \
            $BMNET_OUT_PATH/${net}_output.bin 0
  ) || print_error_bmnet " ${net}<$3,$4,$5,$6> failed" || return $?

  echo " ${net}<$3,$4,$5,$6> passed"
}

function run_test_bmnet_bm1880()
{
  test_bmnet_bm1880 mtcnn det1 1 3 12 12                 || return $?
  test_bmnet_bm1880 mobilenet mobilenet 1 3 224 224      || return $?
  echo "test_bmnet_bm1880 success"
}

run_test_bmnet_bm1880
