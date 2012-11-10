/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_rtcd.h"
#include "dequantize.h"
#include "vp9/common/idct.h"
#include "vpx_mem/vpx_mem.h"
#include "onyxd_int.h"

#ifdef DEC_DEBUG
extern int dec_debug;
#endif

static void add_residual(const int16_t *diff, const uint8_t *pred, int pitch,
                         uint8_t *dest, int stride, int width, int height) {
  int r, c;

  for (r = 0; r < height; r++) {
    for (c = 0; c < width; c++) {
      int a = diff[c] + pred[c];

      if (a < 0)
        a = 0;
      else if (a > 255)
        a = 255;

      dest[c] = (uint8_t) a;
    }

    dest += stride;
    diff += width;
    pred += pitch;
  }
}

static void add_constant_residual(const int16_t diff, const uint8_t *pred,
                                  int pitch, uint8_t *dest, int stride,
                                  int width, int height) {
  int r, c;

  for (r = 0; r < height; r++) {
    for (c = 0; c < width; c++) {
      int a = diff + pred[c];

      if (a < 0)
        a = 0;
      else if (a > 255)
        a = 255;

      dest[c] = (uint8_t) a;
    }

    dest += stride;
    pred += pitch;
  }
}

void vp9_dequantize_b_c(BLOCKD *d) {

  int i;
  int16_t *DQ  = d->dqcoeff;
  int16_t *Q   = d->qcoeff;
  int16_t *DQC = d->dequant;

  for (i = 0; i < 16; i++) {
    DQ[i] = Q[i] * DQC[i];
  }
}


void vp9_ht_dequant_idct_add_c(TX_TYPE tx_type, int16_t *input, int16_t *dq,
                               uint8_t *pred, uint8_t *dest,
                               int pitch, int stride) {
  int16_t output[16];
  int16_t *diff_ptr = output;
  int i;

  for (i = 0; i < 16; i++) {
    input[i] = dq[i] * input[i];
  }

  vp9_ihtllm_c(input, output, 4 << 1, tx_type, 4);

  vpx_memset(input, 0, 32);

  add_residual(diff_ptr, pred, pitch, dest, stride, 4, 4);
}

void vp9_ht_dequant_idct_add_8x8_c(TX_TYPE tx_type, int16_t *input, int16_t *dq,
                                   uint8_t *pred, uint8_t *dest,
                                   int pitch, int stride) {
  int16_t output[64];
  int16_t *diff_ptr = output;
  int i;

  input[0] = dq[0] * input[0];
  for (i = 1; i < 64; i++) {
    input[i] = dq[1] * input[i];
  }

  vp9_ihtllm_c(input, output, 16, tx_type, 8);

  vpx_memset(input, 0, 128);

  add_residual(diff_ptr, pred, pitch, dest, stride, 8, 8);
}

void vp9_dequant_idct_add_c(int16_t *input, int16_t *dq, uint8_t *pred,
                            uint8_t *dest, int pitch, int stride) {
  int16_t output[16];
  int16_t *diff_ptr = output;
  int i;

  for (i = 0; i < 16; i++) {
    input[i] = dq[i] * input[i];
  }

  /* the idct halves ( >> 1) the pitch */
  vp9_short_idct4x4llm_c(input, output, 4 << 1);

  vpx_memset(input, 0, 32);

  add_residual(diff_ptr, pred, pitch, dest, stride, 4, 4);
}

void vp9_dequant_dc_idct_add_c(int16_t *input, int16_t *dq, uint8_t *pred,
                               uint8_t *dest, int pitch, int stride,
                               int Dc) {
  int i;
  int16_t output[16];
  int16_t *diff_ptr = output;

  input[0] = (int16_t)Dc;

  for (i = 1; i < 16; i++) {
    input[i] = dq[i] * input[i];
  }

  /* the idct halves ( >> 1) the pitch */
  vp9_short_idct4x4llm_c(input, output, 4 << 1);

  vpx_memset(input, 0, 32);

  add_residual(diff_ptr, pred, pitch, dest, stride, 4, 4);
}

#if CONFIG_LOSSLESS
void vp9_dequant_idct_add_lossless_c(int16_t *input, int16_t *dq,
                                     uint8_t *pred, uint8_t *dest,
                                     int pitch, int stride) {
  int16_t output[16];
  int16_t *diff_ptr = output;
  int i;

  for (i = 0; i < 16; i++) {
    input[i] = dq[i] * input[i];
  }

  vp9_short_inv_walsh4x4_x8_c(input, output, 4 << 1);

  vpx_memset(input, 0, 32);

  add_residual(diff_ptr, pred, pitch, dest, stride, 4, 4);
}

void vp9_dequant_dc_idct_add_lossless_c(int16_t *input, int16_t *dq,
                                        uint8_t *pred,
                                        uint8_t *dest,
                                        int pitch, int stride, int dc) {
  int i;
  int16_t output[16];
  int16_t *diff_ptr = output;

  input[0] = (int16_t)dc;

  for (i = 1; i < 16; i++) {
    input[i] = dq[i] * input[i];
  }

  vp9_short_inv_walsh4x4_x8_c(input, output, 4 << 1);
  vpx_memset(input, 0, 32);

  add_residual(diff_ptr, pred, pitch, dest, stride, 4, 4);
}
#endif

void vp9_dequantize_b_2x2_c(BLOCKD *d) {
  int i;
  int16_t *DQ  = d->dqcoeff;
  int16_t *Q   = d->qcoeff;
  int16_t *DQC = d->dequant;

  for (i = 0; i < 16; i++) {
    DQ[i] = (int16_t)((Q[i] * DQC[i]));
  }
#ifdef DEC_DEBUG
  if (dec_debug) {
    int j;
    printf("Dequantize 2x2\n");
    for (j = 0; j < 16; j++) printf("%d ", Q[j]);
    printf("\n");
    for (j = 0; j < 16; j++) printf("%d ", DQ[j]);
    printf("\n");
  }
#endif
}

void vp9_dequant_idct_add_8x8_c(int16_t *input, int16_t *dq, uint8_t *pred,
                                uint8_t *dest, int pitch, int stride,
                                int dc, uint16_t eobs) {
  int16_t output[64];
  int16_t *diff_ptr = output;
  int i;

#ifdef DEC_DEBUG
  if (dec_debug) {
    int j;
    printf("Input 8x8\n");
    for (j = 0; j < 64; j++) {
      printf("%d ", input[j]);
      if (j % 8 == 7) printf("\n");
    }
  }
#endif

  /* If dc is 1, then input[0] is the reconstructed value, do not need
   * dequantization. Also, when dc is 1, dc is counted in eobs, namely eobs >=1.
   */
  if (!dc)
    input[0] *= dq[0];

  /* The calculation can be simplified if there are not many non-zero dct
   * coefficients. Use eobs to decide what to do.
   * TODO(yunqingwang): "eobs = 1" case is also handled in vp9_short_idct8x8_c.
   * Combine that with code here.
   */
  if (eobs == 0) {
    /* All 0 DCT coefficient */
    vp9_copy_mem8x8(pred, pitch, dest, stride);
  } else if (eobs == 1) {
    /* DC only DCT coefficient. */
    int16_t out;

    /* Note: the idct1 will need to be modified accordingly whenever
     * vp9_short_idct8x8_c() is modified. */
    out = (input[0] + 1 + (input[0] < 0)) >> 2;
    out = out << 3;
    out = (out + 32) >> 7;

    input[0] = 0;

    add_constant_residual(out, pred, pitch, dest, stride, 8, 8);
  } else if (eobs <= 10) {
    input[1] = input[1] * dq[1];
    input[2] = input[2] * dq[1];
    input[3] = input[3] * dq[1];
    input[8] = input[8] * dq[1];
    input[9] = input[9] * dq[1];
    input[10] = input[10] * dq[1];
    input[16] = input[16] * dq[1];
    input[17] = input[17] * dq[1];
    input[24] = input[24] * dq[1];

    vp9_short_idct10_8x8_c(input, output, 16);

    input[0] = input[1] = input[2] = input[3] = 0;
    input[8] = input[9] = input[10] = 0;
    input[16] = input[17] = 0;
    input[24] = 0;

    add_residual(diff_ptr, pred, pitch, dest, stride, 8, 8);
  } else {
    // recover quantizer for 4 4x4 blocks
    for (i = 1; i < 64; i++) {
      input[i] = input[i] * dq[1];
    }
#ifdef DEC_DEBUG
  if (dec_debug) {
    int j;
    printf("Input DQ 8x8\n");
    for (j = 0; j < 64; j++) {
      printf("%d ", input[j]);
      if (j % 8 == 7) printf("\n");
    }
  }
#endif

    // the idct halves ( >> 1) the pitch
    vp9_short_idct8x8_c(input, output, 16);
#ifdef DEC_DEBUG
  if (dec_debug) {
    int j;
    printf("Output 8x8\n");
    for (j = 0; j < 64; j++) {
      printf("%d ", output[j]);
      if (j % 8 == 7) printf("\n");
    }
  }
#endif

    vpx_memset(input, 0, 128);

    add_residual(diff_ptr, pred, pitch, dest, stride, 8, 8);

#ifdef DEC_DEBUG
  if (dec_debug) {
    int k, j;
    printf("Final 8x8\n");
    for (j = 0; j < 8; j++) {
      for (k = 0; k < 8; k++) {
        printf("%d ", origdest[k]);
      }
      printf("\n");
      origdest += stride;
    }
  }
#endif
  }
}

void vp9_ht_dequant_idct_add_16x16_c(TX_TYPE tx_type, int16_t *input,
                                     int16_t *dq, uint8_t *pred, uint8_t *dest,
                                     int pitch, int stride) {
  int16_t output[256];
  int16_t *diff_ptr = output;
  int i;

  input[0]= input[0] * dq[0];

  // recover quantizer for 4 4x4 blocks
  for (i = 1; i < 256; i++)
    input[i] = input[i] * dq[1];

  // inverse hybrid transform
  vp9_ihtllm_c(input, output, 32, tx_type, 16);

  // the idct halves ( >> 1) the pitch
  // vp9_short_idct16x16_c(input, output, 32);

  vpx_memset(input, 0, 512);

  add_residual(diff_ptr, pred, pitch, dest, stride, 16, 16);
}

void vp9_dequant_idct_add_16x16_c(int16_t *input, int16_t *dq, uint8_t *pred,
                                  uint8_t *dest, int pitch, int stride,
                                  uint16_t eobs) {
  int16_t output[256];
  int16_t *diff_ptr = output;
  int i;

  /* The calculation can be simplified if there are not many non-zero dct
   * coefficients. Use eobs to separate different cases. */
  if (eobs == 0) {
    /* All 0 DCT coefficient */
    vp9_copy_mem16x16(pred, pitch, dest, stride);
  } else if (eobs == 1) {
    /* DC only DCT coefficient. */
    int16_t out;

    /* Note: the idct1 will need to be modified accordingly whenever
     * vp9_short_idct16x16_c() is modified. */
    out = (input[0] * dq[0] + 2) >> 2;
    out = (out + 2) >> 2;
    out = (out + 4) >> 3;

    input[0] = 0;

    add_constant_residual(out, pred, pitch, dest, stride, 16, 16);
  } else if (eobs <= 10) {
    input[0]= input[0] * dq[0];
    input[1] = input[1] * dq[1];
    input[2] = input[2] * dq[1];
    input[3] = input[3] * dq[1];
    input[16] = input[16] * dq[1];
    input[17] = input[17] * dq[1];
    input[18] = input[18] * dq[1];
    input[32] = input[32] * dq[1];
    input[33] = input[33] * dq[1];
    input[48] = input[48] * dq[1];

    // the idct halves ( >> 1) the pitch
    vp9_short_idct10_16x16_c(input, output, 32);

    input[0] = input[1] = input[2] = input[3] = 0;
    input[16] = input[17] = input[18] = 0;
    input[32] = input[33] = 0;
    input[48] = 0;

    add_residual(diff_ptr, pred, pitch, dest, stride, 16, 16);
  } else {
    input[0]= input[0] * dq[0];

    // recover quantizer for 4 4x4 blocks
    for (i = 1; i < 256; i++)
      input[i] = input[i] * dq[1];

    // the idct halves ( >> 1) the pitch
    vp9_short_idct16x16_c(input, output, 32);

    vpx_memset(input, 0, 512);

    add_residual(diff_ptr, pred, pitch, dest, stride, 16, 16);
  }
}
