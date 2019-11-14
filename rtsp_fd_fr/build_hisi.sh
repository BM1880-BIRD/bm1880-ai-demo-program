#!/bin/bash

cmake -DPROGRAMS="yolo_hisi" -DPLATFORM=hisiv300 . -Bbuild/hisiv300
make -j8 -C build/hisiv300

