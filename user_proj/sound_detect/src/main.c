#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <packet.h>
#include <sleep.h>
#include <network.h>
#include <string.h>
#include <tinimpi.h>
#include <stdio.h>
#include <stdint.h>
#include <topology.h>

#include "data.h"

rank_t rank;

#define NUM_SAMPLES 256
#define SAMPLE_FREQ 32000

rank_t get_next_rank() {
  return (rank + 1) % 2;
}

rank_t get_prev_rank() {
  if (rank == 0) {
    return 1;
  }
  return rank - 1;
}

void sounddetect_ref_pair(
  // int num_samples,
  // int sample_freq,
  // int nproc
) {
  // (void)num_samples, sample_freq;
  int16_t all_mic_data[4][NUM_SAMPLES];

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < NUM_SAMPLES; j++) {
      all_mic_data[i][j] = 0;
    }
  }

  // Simplifying from 24 bits to 16 bits
  all_mic_data[rank][0] = rank;
  for (int i = 1; i < NUM_SAMPLES; i++) {
    all_mic_data[rank][i] = raw_data[i][rank];
  }

  int next_rank = get_next_rank();
  int prev_rank = get_prev_rank();
  // Send data to all nodes
  // initiate sending data
  tinimpi_send2(next_rank, 0, (uint8_t*)all_mic_data[rank], NUM_SAMPLES * sizeof(int16_t));
  int done = 0;
  while (!done) {
    int16_t recv_buffer[NUM_SAMPLES];
    uint16_t out_len;
    tinimpi_recv2(prev_rank, 0, (uint8_t*)recv_buffer, NUM_SAMPLES * sizeof(int16_t), &out_len);

    int orig_src = recv_buffer[0];
    if (orig_src == rank) {
      done = 1;
    } else {
      if (rank != 0) {
        memcpy(all_mic_data[orig_src], recv_buffer,
               NUM_SAMPLES * sizeof(int16_t));
      }
      tinimpi_send2(next_rank, 0, (uint8_t *)recv_buffer, NUM_SAMPLES * sizeof(int16_t));
      done = 0;
    }
  }
  tinimpi_barrier2();

  // // Find Time Lag
  // std::vector<float> best_dist_diff(nproc, 0.0f);
  // bool silent = true;

  // for (int i = 1; i < nproc; i++) {
  //   int64_t energy = 0;
  //   int best_lag = 0;
  //   int64_t best_score = INT64_MIN;
  //   for (int lag = -MAX_LAG; lag < MAX_LAG; lag++) {
  //     int64_t score = 0;
  //     for (int j = 1; j < send_size; j++) {
  //       energy += (int64_t)all_mic_data[0][j] * (int64_t)all_mic_data[0][j];
  //       if (j + lag < 0) {
  //         continue;
  //       }
  //       if (j + lag >= send_size) {
  //         continue;
  //       }
  //       score += (int64_t)all_mic_data[i][j + lag] * (int64_t)all_mic_data[0][j];
  //     }
  //     if (score > best_score) {
  //       best_score = score;
  //       best_lag = lag;
  //     }
  //   }
  //   if (energy > ENERGY_THRESHOLD) {
  //     silent = false;
  //   }
  //   best_dist_diff[i] = -(float)best_lag * (1.0f/sample_freq) * 343.0f;
  //   if (rank == 1) {
  //     // printf("energy: %ld\n", energy);
  //     // printf("pair (0, %d): best_lag=%d dist_diff=%f\n", i, best_lag, best_dist_diff[i]);
  //   }
  // }

  // // Find intersection
  // float best_error = INFINITY;
  // float best_x = 0.0f;
  // float best_y = 0.0f;
  // for (float x = X_MIN; x < X_MAX; x += DX) {
  //   for (float y = Y_MIN; y < Y_MAX; y += DY) {
  //     float total_error = 0.0f;

  //     float dx_0 = x - mic_positions[0][0];
  //     float dy_0 = y - mic_positions[0][1];
  //     float dist_0 = sqrtf(dx_0 * dx_0 + dy_0 * dy_0);

  //     for (int i = 1; i < nproc; i++) {
  //       float dx_i = x - mic_positions[i][0];
  //       float dy_i = y - mic_positions[i][1];
  //       float dist_i = sqrtf(dx_i * dx_i + dy_i * dy_i);

  //       float error = dist_0 - dist_i - best_dist_diff[i];
  //       total_error += error * error;
  //     }

  //     if (total_error < best_error) {
  //       best_error = total_error;
  //       best_x = x;
  //       best_y = y;
  //     }
  //   }
  // }

  // if (rank == 1) {
  //   if (silent) {
  //     std::cout << "silent" << std::endl;
  //   } else {
  //     std::cout << "mic position: " << mic_positions[1][0] << ", " << mic_positions[1][1] << std::endl;
  //     std::cout << "coord: " << best_x << ", " << best_y << std::endl;
  //   }
  //   // std::cout << "dx, dy: " << DX << ", " << DY << std::endl;
  // }
}


void computation() {
  tag_t t = 16;

  sleep(5000);  

  sounddetect_ref_pair();
  
  while (1);
}

int main(UNUSED int argc, UNUSED char const *argv[]) {
  tinimpi_init(&rank, &computation);
  while(1);
}
