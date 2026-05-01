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
#include <math.h>
#include <topology.h>

#include "data.h"

rank_t rank;

#define NUM_SAMPLES 256
#define SAMPLE_FREQ 32000

#define MAX_LAG 25
#define ENERGY_THRESHOLD 20000000000LL

#define X_MIN -1.0f
#define X_MAX 1.0f
#define Y_MIN -1.0f
#define Y_MAX 1.0f
#define GRID_WIDTH 200
#define GRID_HEIGHT 200

#define DX (X_MAX - X_MIN) / (GRID_WIDTH - 1)
#define DY (Y_MAX - Y_MIN) / (GRID_HEIGHT - 1)

struct Result {
  float error;
  float x;
  float y;
  uint8_t src;
};

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
  int offset,
  uint32_t *total_communication_time,
  uint32_t *total_computation_time
  // int num_samples,
  // int sample_freq,
  // int nproc
) {
  // (void)num_samples, sample_freq;
  uint32_t comp_start = get_time();
  int16_t all_mic_data[4][NUM_SAMPLES];

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < NUM_SAMPLES; j++) {
      all_mic_data[i][j] = 0;
    }
  }

  // Simplifying from 24 bits to 16 bits
  all_mic_data[rank][0] = rank;
  for (int i = 1; i < NUM_SAMPLES + 1; i++) {
    all_mic_data[rank][i] = (int16_t)(raw_data[i + offset][rank] >> 8);
  }
  *total_computation_time += (get_time() - comp_start);

  uint32_t comm_start = get_time();
  int next_rank = get_next_rank();
  int prev_rank = get_prev_rank();

  int16_t recv_buffer[NUM_SAMPLES];
  uint16_t out_len;
  tinimpi_req_t sreq, rreq;

  tinimpi_isend(next_rank, 0, (uint8_t*)all_mic_data[rank], NUM_SAMPLES * sizeof(int16_t), &sreq);
  tinimpi_irecv(prev_rank, 0, (uint8_t*)recv_buffer, NUM_SAMPLES * sizeof(int16_t), &out_len, &rreq);
  tinimpi_wait(&sreq);
  tinimpi_wait(&rreq);

  int done = 0;
  while (!done) {
    int orig_src = recv_buffer[0];
    if (orig_src == rank) {
      done = 1;
    } else {
      memcpy(all_mic_data[orig_src], recv_buffer, NUM_SAMPLES * sizeof(int16_t));
      tinimpi_isend(next_rank, 0, (uint8_t*)recv_buffer, NUM_SAMPLES * sizeof(int16_t), &sreq);
      tinimpi_irecv(prev_rank, 0, (uint8_t*)recv_buffer, NUM_SAMPLES * sizeof(int16_t), &out_len, &rreq);
      tinimpi_wait(&sreq);
      tinimpi_wait(&rreq);
    }
  }
  tinimpi_barrier2();
  *total_communication_time += (get_time() - comm_start);

  // // Find Time Lag
  // std::vector<float> best_dist_diff(nproc, 0.0f);
  comp_start = get_time();
  float best_dist_diff[2];
  int silent = 1;
  uint16_t send_size = NUM_SAMPLES + 1;

  for (int i = 1; i < TOPOLOGY; i++) {
    int64_t energy = 0;
    int best_lag = 0;
    int64_t best_score = INT64_MIN;
    for (int lag = -MAX_LAG; lag < MAX_LAG; lag++) {
      int64_t score = 0LL;
      for (int j = 1; j < send_size; j++) {
        // printf("mic 0 data: %d\r\n", all_mic_data[0][j]);
        energy += (int64_t)all_mic_data[0][j] * (int64_t)all_mic_data[0][j];
        if (j + lag < 0) {
          continue;
        }
        if (j + lag >= send_size) {
          continue;
        }
        score += (int64_t)all_mic_data[i][j + lag] * (int64_t)all_mic_data[0][j];
      }
      if (score > best_score) {
        best_score = score;
        best_lag = lag;
      }
    }
    if (energy > ENERGY_THRESHOLD) {
      silent = 0;
    }
    best_dist_diff[i] = -(float)best_lag * (1.0f/SAMPLE_FREQ) * 343.0f;
    if (rank == 1) {
      int32_t high = (int32_t)(energy >> 32);
      uint32_t low = (uint32_t)(energy & 0xFFFFFFFF);

      printf("energy: %ld%08lu\r\n", high, low);
      // printf("energy: %lld\n", energy);
      // printf("pair (0, %d): best_lag=%d dist_diff=%f\n", i, best_lag, best_dist_diff[i]);
    }
  }

  // // Find intersection
  float best_error = INFINITY;
  float best_x = 0.0f;
  float best_y = 0.0f;
  
  int nx = GRID_WIDTH / TOPOLOGY;
  int start_ix = rank * nx;
  int end_ix = (rank + 1) * nx;

  for (int ix = start_ix; ix < end_ix; ix ++) {
    float x = X_MIN + ix * DX;
    for (float y = Y_MIN; y < Y_MAX; y += DY) {
      float total_error = 0.0f;

      float dx_0 = x - mic_positions[0][0];
      float dy_0 = y - mic_positions[0][1];
      float dist_0_sq = dx_0 * dx_0 + dy_0 * dy_0;

      for (int i = 1; i < TOPOLOGY; i++) {
        float dx_i = x - mic_positions[i][0];
        float dy_i = y - mic_positions[i][1];
        float dist_i_sq = dx_i * dx_i + dy_i * dy_i;

        float error = dist_0_sq - dist_i_sq - best_dist_diff[i];
        total_error += error * error;
      }

      if (total_error < best_error) {
        best_error = total_error;
        best_x = x;
        best_y = y;
      }
    }
  }
  *total_computation_time += (get_time() - comp_start);

  comm_start = get_time();
  struct Result local_result = {
    best_error,
    best_x,
    best_y,
    rank,
  };

  struct Result all_results[2];
  struct Result recv_result;
  uint16_t result_out_len = 0;
  tinimpi_req_t result_sreq, result_rreq;

  all_results[rank] = local_result;
  tinimpi_isend(next_rank, 0, (uint8_t*)&local_result, sizeof(struct Result), &result_sreq);
  tinimpi_irecv(prev_rank, 0, (uint8_t*)&recv_result, sizeof(struct Result), &result_out_len, &result_rreq);
  tinimpi_wait(&result_sreq);
  tinimpi_wait(&result_rreq);

  int gather_done = 0;
  while (!gather_done) {
    int orig_src = recv_result.src;
    if (orig_src == rank) {
      gather_done = 1;
    } else {
      memcpy(&all_results[orig_src], &recv_result, sizeof(struct Result));
      tinimpi_isend(next_rank, 0, (uint8_t*)&recv_result, sizeof(struct Result), &result_sreq);
      tinimpi_irecv(prev_rank, 0, (uint8_t*)&recv_result, sizeof(struct Result), &result_out_len, &result_rreq);
      tinimpi_wait(&result_sreq);
      tinimpi_wait(&result_rreq);
    }
  }
  tinimpi_barrier2();
  *total_communication_time += (get_time() - comm_start);

  comp_start = get_time();
  float global_best_error = INFINITY;
  float global_x = 0.0f;
  float global_y = 0.0f;
  for (int i = 0; i < TOPOLOGY; i++) {
    if (all_results[i].error < global_best_error) {
      global_best_error = all_results[i].error;
      global_x = all_results[i].x;
      global_y = all_results[i].y;
    }
  }
  *total_computation_time += (get_time() - comp_start);

  if (rank == 1) {
    if (silent) {
      printf("silent\n");
      // std::cout << "silent" << std::endl;
    } else {
      // printf("coord: (%d, %d)\n", (int)(best_x * 100), (int)(best_y * 100));
      printf("coord: (%d, %d)\n", (int)(global_x * 100), (int)(global_y * 100));
      // std::cout << "mic position: " << mic_positions[1][0] << ", " << mic_positions[1][1] << std::endl;
      // std::cout << "coord: " << best_x << ", " << best_y << std::endl;
    }
    // std::cout << "dx, dy: " << DX << ", " << DY << std::endl;
  }
}


void computation() {
  tag_t t = 16;

  sleep(5000);  

  uint32_t comp_time = 0;
  uint32_t comm_time = 0;
  uint32_t start_time = get_time();
  for (int i = 0; i < 3840; i += 256) {
    sounddetect_ref_pair(i, &comm_time, &comp_time);
  }
  uint32_t total_time = get_time() - start_time;
  printf("total time: %ldms\n", total_time);
  printf("compute time: %ldms\n", comp_time);
  printf("communication time: %ldms\n", comm_time);
  
  while (1);
}

int main(UNUSED int argc, UNUSED char const *argv[]) {
  tinimpi_init(&rank, &computation);
  while(1);
}
