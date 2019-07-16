#ifndef STAN_MATH_OPENCL_KERNELS_MATRIX_MULTIPLY_HPP
#define STAN_MATH_OPENCL_KERNELS_MATRIX_MULTIPLY_HPP
#ifdef STAN_OPENCL

#include <stan/math/opencl/kernel_cl.hpp>
#include <stan/math/opencl/buffer_types.hpp>

namespace stan {
namespace math {
namespace opencl_kernels {
// \cond
static const char* matrix_multiply_kernel_code = STRINGIFY(
    // \endcond
    /**
     * Matrix multiplication on the OpenCL device
     *
     * @param[in] A the left matrix in matrix multiplication
     * @param[in] B the right matrix in matrix multiplication
     * @param[out] C the output matrix
     * @param[in] M Number of rows for matrix A
     * @param[in] N Number of cols for matrix B
     * @param[in] K Number of cols for matrix A and number of rows for matrix B
     * @param[in] lower_upper_A the triangularity of A (lower, upper or none)
     * @param[in] lower_upper_B the triangularity of B (lower, upper or none)
     */
    __kernel void matrix_multiply(
        const __global double* A, const __global double* B, __global double* C,
        const int M, const int N, const int K, unsigned int lower_upper_A,
        unsigned int lower_upper_B) {
      // thread index inside the thread_block
      const int row_in_block = get_local_id(0);
      const int col_in_block = get_local_id(1);
      // global thread index
      const int i = THREAD_BLOCK_SIZE * get_group_id(0) + row_in_block;
      const int j = THREAD_BLOCK_SIZE * get_group_id(1) + col_in_block;
      // identify if the matrix multiply is split
      const int split_id = get_global_id(2);
      const int split_size = get_global_size(2);
      // local memory
      __local double A_local[THREAD_BLOCK_SIZE][THREAD_BLOCK_SIZE];
      __local double B_local[THREAD_BLOCK_SIZE][THREAD_BLOCK_SIZE];

      double acc[WORK_PER_THREAD];
      for (int w = 0; w < WORK_PER_THREAD; w++) {
        acc[w] = 0.0;
      }
      // the number of tiles for each scalar product in the matrix mulitply
      const int num_tiles = (K + THREAD_BLOCK_SIZE - 1) / THREAD_BLOCK_SIZE;
      // in case of splitting the matrix multiply we need
      // use split_offset_tiles the threads assigned part
      // of the scalar products, while the split_tiles
      // determines the number of tiles a thread multiplies
      // if split_size = 1, each thread calculates the
      // the entire scalar product for all assigned
      // elements of the resulting matrix, meaning that
      // split_offset_tiles is 0 and split_tiles = num_tiles
      int split_tiles = num_tiles / split_size;
      const int split_remainder = num_tiles % split_size;
      int split_offset_tiles = split_id * split_tiles;
      if (split_id < split_remainder) {
        split_offset_tiles = split_offset_tiles + split_id;
        split_tiles++;
      } else {
        split_offset_tiles = split_offset_tiles + split_remainder;
      }
      // This kernel is based on the well known
      // general matrix multiplication kernels that
      // use tiling for shared memory
      // In cases where a matrix is lower triangular
      // its not necessary to multiply the elements
      // over the diagonal, therefore those tiles
      // in the matrix multiply can be skipped.
      // With upper triangular matrices we dont need
      // to multiply the elements under the diagonal,
      // so those tiles can be skipped.
      // The following code determines the start and
      // end tile based on triangularity of the input matrices
      // If no matrices are triangular the starting tile
      // is 0 and the end tile is num_tiles-1 which
      // is then a general matrix multiply
      const int end_tile_A
          = lower_upper_A == LOWER ? (i / THREAD_BLOCK_SIZE) : (num_tiles - 1);
      const int end_tile_B
          = lower_upper_B == UPPER ? (j / THREAD_BLOCK_SIZE) : (num_tiles - 1);
      const int start_tile_A
          = lower_upper_A == UPPER ? (i / THREAD_BLOCK_SIZE) : 0;
      const int start_tile_B
          = lower_upper_B == LOWER ? (j / THREAD_BLOCK_SIZE) : 0;
      // the starting and end tiles for a thread are determined by
      // split_offset_tiles and split_tiles. If the input matrix is
      // triangular some tiles can be skipped in which case we
      // either start the scalar product at larger cols/rows
      // or end them at smaller cols/rows.
      int start_tile = max(start_tile_A, start_tile_B);
      start_tile = max(start_tile, split_offset_tiles);
      int end_tile = min(end_tile_A, end_tile_B);                      // NOLINT
      end_tile = min(end_tile, split_offset_tiles + split_tiles - 1);  // NOLINT

      const int total_work_n = min(THREAD_BLOCK_SIZE, N - THREAD_BLOCK_SIZE * (int)get_group_id(1));
      const int total_work_m = min(THREAD_BLOCK_SIZE, M - THREAD_BLOCK_SIZE * (int)get_group_id(0));
      const int total_work_nm = total_work_n * total_work_m;
      const int threads_in_block = THREAD_BLOCK_SIZE_COL*THREAD_BLOCK_SIZE;
      const int linear_index = get_local_id(0) + get_local_id(1) * THREAD_BLOCK_SIZE;

      //special handling of first block
      if(start_tile <= end_tile && (lower_upper_A == UPPER || lower_upper_B == LOWER)){
//        printf("1");
        const int tiled_i = THREAD_BLOCK_SIZE * start_tile + row_in_block;
        const int tiled_j = THREAD_BLOCK_SIZE * start_tile + col_in_block;
        for (int w = 0; w < WORK_PER_THREAD; w++) {
          // For the tiles on the diagonal we can ignore the values over
          // the diagonal if the matrix is lower triangular or under
          // the diagonal if the matrix is upper triangular
          const int A_curr_j = tiled_j + w * THREAD_BLOCK_SIZE_COL;
          const int B_curr_j = j + w * THREAD_BLOCK_SIZE_COL;
          const int curr_k = col_in_block + w * THREAD_BLOCK_SIZE_COL;
          // check if the indexes are outside the matrix
          // or under/above the diagonal with upper/lower
          // triangular matrices
          if (A_curr_j >= K || i >= M
              || (lower_upper_A == LOWER && A_curr_j > i)
              || (lower_upper_A == UPPER && A_curr_j < i)) {
            A_local[curr_k][row_in_block] = 0.0;
          } else {
            A_local[curr_k][row_in_block] = A[A_curr_j * M + i];
          }
          if (B_curr_j >= N || tiled_i >= K
              || (lower_upper_B == LOWER && B_curr_j > tiled_i)
              || (lower_upper_B == UPPER && B_curr_j < tiled_i)) {
            B_local[curr_k][row_in_block] = 0.0;
          } else {
            B_local[curr_k][row_in_block] = B[B_curr_j * K + tiled_i];
          }
        }
//        printf("2");
        barrier(CLK_LOCAL_MEM_FENCE);
        const int total_work_k = min(THREAD_BLOCK_SIZE, K - THREAD_BLOCK_SIZE * start_tile);
        for (int idx = linear_index, w = 0; idx < total_work_nm; idx += threads_in_block, w++) {
          const int row_B_local = idx / total_work_m;
          const int col_A_local = idx % total_work_m;
          for (int idx_in_block = 0; idx_in_block < total_work_k; idx_in_block++) {
            acc[w] += A_local[idx_in_block][col_A_local]
                            * B_local[row_B_local][idx_in_block];
          }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        start_tile++;
      }
      //special handling of last block
      if(start_tile <= end_tile && (lower_upper_A == LOWER || lower_upper_B == UPPER || K % THREAD_BLOCK_SIZE != 0)){
        const int tiled_i = THREAD_BLOCK_SIZE * end_tile + row_in_block;
        const int tiled_j = THREAD_BLOCK_SIZE * end_tile + col_in_block;
        for (int w = 0; w < WORK_PER_THREAD; w++) {
          // For the tiles on the diagonal we can ignore the values over
          // the diagonal if the matrix is lower triangular or under
          // the diagonal if the matrix is upper triangular
          const int A_curr_j = tiled_j + w * THREAD_BLOCK_SIZE_COL;
          const int B_curr_j = j + w * THREAD_BLOCK_SIZE_COL;
          const int curr_k = col_in_block + w * THREAD_BLOCK_SIZE_COL;
          // check if the indexes are outside the matrix
          // or under/above the diagonal with upper/lower
          // triangular matrices
          if (A_curr_j >= K || i >= M
              || (lower_upper_A == LOWER && A_curr_j > i)
              || (lower_upper_A == UPPER && A_curr_j < i)) {
            A_local[curr_k][row_in_block] = 0.0;
          } else {
            A_local[curr_k][row_in_block] = A[A_curr_j * M + i];
          }
          if (B_curr_j >= N || tiled_i >= K
              || (lower_upper_B == LOWER && B_curr_j > tiled_i)
              || (lower_upper_B == UPPER && B_curr_j < tiled_i)) {
            B_local[curr_k][row_in_block] = 0.0;
          } else {
            B_local[curr_k][row_in_block] = B[B_curr_j * K + tiled_i];
          }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        const int total_work_k = min(THREAD_BLOCK_SIZE, K - THREAD_BLOCK_SIZE * end_tile);
        for (int idx = linear_index, w = 0; idx < total_work_nm; idx += threads_in_block, w++) {
          const int row_B_local = idx / total_work_m;
          const int col_A_local = idx % total_work_m;
          for (int idx_in_block = 0; idx_in_block < total_work_k; idx_in_block++) {
//            printf("t %d i %d += A(%d,%d) * B(%d,%d) - %lf * %lf\n",linear_index, w ,row_B_local, idx_in_block,
//                   idx_in_block, col_A_local, A_local[idx_in_block][row_B_local], B_local[col_A_local][idx_in_block]);
            acc[w] += A_local[idx_in_block][col_A_local]
                            * B_local[row_B_local][idx_in_block];
          }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        end_tile--;
      }
      //special handling of edge blocks
//      printf("3");
      if(total_work_n < THREAD_BLOCK_SIZE || total_work_m < THREAD_BLOCK_SIZE) {
//        printf("4");
        for (int tile_idx = start_tile; tile_idx <= end_tile; tile_idx++) {
//          printf("-");
          const int tiled_i = THREAD_BLOCK_SIZE * tile_idx + row_in_block;
          const int tiled_j = THREAD_BLOCK_SIZE * tile_idx + col_in_block;
          // each thread copies WORK_PER_THREAD values to the local
          // memory
          for (int w = 0; w < WORK_PER_THREAD; w++) {
            const int A_curr_j = tiled_j + w * THREAD_BLOCK_SIZE_COL;
            const int B_curr_j = j + w * THREAD_BLOCK_SIZE_COL;
            const int curr_k = col_in_block + w * THREAD_BLOCK_SIZE_COL;
            // check if the indexes are outside the matrix
            if (i < M) {
              A_local[curr_k][row_in_block] = A[A_curr_j * M + i];
            }
            if (B_curr_j < N) {
              B_local[curr_k][row_in_block] = B[B_curr_j * K + tiled_i];
            }
          }
          barrier(CLK_LOCAL_MEM_FENCE);
          int total_work_k = min(THREAD_BLOCK_SIZE, K - THREAD_BLOCK_SIZE * tile_idx);
          for (int idx = linear_index, w = 0; idx < total_work_nm; idx += threads_in_block, w++) {
            const int row_B_local = idx / total_work_m;
            const int col_A_local = idx % total_work_m;
            for (int idx_in_block = 0; idx_in_block < total_work_k; idx_in_block++) {
              acc[w] += A_local[idx_in_block][col_A_local]
                              * B_local[row_B_local][idx_in_block];
            }
          }
          barrier(CLK_LOCAL_MEM_FENCE);
        }
//        printf("5\n");
        for(int idx = linear_index, w=0; idx < total_work_nm; idx += threads_in_block, w++){
          const int curr_i = THREAD_BLOCK_SIZE * get_group_id(0) + idx % total_work_m;
          const int B_curr_j = THREAD_BLOCK_SIZE * get_group_id(1) + idx / total_work_m;
//          printf("t %d, i %d writing at %d %d:   %lf\n",linear_index, w, curr_i, B_curr_j, acc[w]);
          C[split_id * M * N + B_curr_j * M + curr_i] = acc[w];
        }
      }
      //general case that is not on the edge - all threads have work
      else{
        for (int tile_idx = start_tile; tile_idx <= end_tile; tile_idx++) {
          const int tiled_i = THREAD_BLOCK_SIZE * tile_idx + row_in_block;
          const int tiled_j = THREAD_BLOCK_SIZE * tile_idx + col_in_block;
          // each thread copies WORK_PER_THREAD values to the local
          // memory
          for (int w = 0; w < WORK_PER_THREAD; w++) {
            const int A_curr_j = tiled_j + w * THREAD_BLOCK_SIZE_COL;
            const int B_curr_j = j + w * THREAD_BLOCK_SIZE_COL;
            const int curr_k = col_in_block + w * THREAD_BLOCK_SIZE_COL;
            A_local[curr_k][row_in_block] = A[A_curr_j * M + i];
            B_local[curr_k][row_in_block] = B[B_curr_j * K + tiled_i];
          }
          barrier(CLK_LOCAL_MEM_FENCE);
          for(int w = 0; w < WORK_PER_THREAD;  w++){
            for (int idx_in_block = 0; idx_in_block < THREAD_BLOCK_SIZE; idx_in_block++) {
                acc[w] += A_local[idx_in_block][row_in_block]
                          * B_local[w * THREAD_BLOCK_SIZE_COL + col_in_block][idx_in_block];
            }
          }
          barrier(CLK_LOCAL_MEM_FENCE);
        }
        // each thread saves WORK_PER_THREAD values
        for(int w = 0; w < WORK_PER_THREAD;  w++){
//          const int curr_i = THREAD_BLOCK_SIZE * get_group_id(0) + w * THREAD_BLOCK_SIZE_COL + col_in_block;
          const int curr_j = j + w * THREAD_BLOCK_SIZE_COL;
          C[split_id * M * N + curr_j * M + i] = acc[w];
        }
      }
    }
    // \cond
);
// \endcond

/**
 * See the docs for \link kernels/matrix_multiply.hpp matrix_multiply() \endlink
 */
const kernel_cl<in_buffer, in_buffer, out_buffer, int, int, int,
                TriangularViewCL, TriangularViewCL>
    matrix_multiply("matrix_multiply",
                    {thread_block_helpers, matrix_multiply_kernel_code},
                    {{"THREAD_BLOCK_SIZE", 32}, {"WORK_PER_THREAD", 8}});

// \cond
static const char* matrix_vector_multiply_kernel_code = STRINGIFY(
    // \endcond
    /**
     * Matrix-vector multiplication R=A*B on the OpenCL device
     *
     * @param[in] A matrix in matrix-vector multiplication
     * @param[in] B vector in matrix-vector multiplication
     * @param[out] R the output vector
     * @param[in] M Number of rows for matrix A
     * @param[in] N Number of cols for matrix A and number of rows for vector B
     * @param[in] lower_upper_A the triangularity of A (lower, upper or none)
     * @param[in] lower_upper_B the triangularity of B (lower, upper or none)
     */
    __kernel void matrix_vector_multiply(
        const __global double* A, const __global double* B, __global double* R,
        const int M, const int N, unsigned int lower_upper_A,
        unsigned int lower_upper_B) {
      const int gid = get_global_id(0);

      const int start = lower_upper_A == UPPER ? gid : 0;
      const int stop
          = lower_upper_B == UPPER ? 1 : (lower_upper_A == LOWER ? gid + 1 : N);

      double acc = 0;
      for (int i = start, j = M * start; i < stop; i++, j += M) {
        acc += A[j + gid] * B[i];
      }
      R[gid] = acc;
    }
    // \cond
);
// \endcond

/**
 * See the docs for \link kernels/matrix_multiply.hpp matrix_vector_multiply()
 * \endlink
 */
const kernel_cl<in_buffer, in_buffer, out_buffer, int, int, TriangularViewCL,
                TriangularViewCL>
    matrix_vector_multiply("matrix_vector_multiply",
                           matrix_vector_multiply_kernel_code);

// \cond
static const char* row_vector_matrix_multiply_kernel_code = STRINGIFY(
    // \endcond
    /**
     * Row vector-matrix multiplication R=A*B on the OpenCL device
     *
     * @param[in] A row vector in row vector-matrix multiplication
     * @param[in] B matrix in row vector-matrix multiplication
     * @param[out] R the output vector
     * @param[in] N Number of cols for row vector A and number of rows for
     * matrix B
     * @param[in] K Number of cols for matrix B
     * @param[in] lower_upper_A the triangularity of A (lower, upper or none)
     * @param[in] lower_upper_B the triangularity of B (lower, upper or none)
     */
    __kernel void row_vector_matrix_multiply(
        const __global double* A, const __global double* B, __global double* R,
        const int N, const int K, unsigned int lower_upper_A,
        unsigned int lower_upper_B) {
      const int lid = get_local_id(0);
      const int gid = get_global_id(0);
      const int wgid = get_group_id(0);

      const int start = lower_upper_B == LOWER ? wgid : 0;
      const int stop = lower_upper_A == LOWER
                           ? 1
                           : (lower_upper_B == UPPER) ? wgid + 1 : N;

      double acc = 0;
      for (int i = lid + start; i < stop; i += LOCAL_SIZE_) {
        acc += A[i] * B[i + wgid * N];
      }

      __local double res_loc[LOCAL_SIZE_];
      res_loc[lid] = acc;
      barrier(CLK_LOCAL_MEM_FENCE);
      for (int step = LOCAL_SIZE_ / REDUCTION_STEP_SIZE; step > 0;
           step /= REDUCTION_STEP_SIZE) {
        if (lid < step) {
          for (int i = 1; i < REDUCTION_STEP_SIZE; i++) {
            res_loc[lid] += res_loc[lid + step * i];
          }
        }
        barrier(CLK_LOCAL_MEM_FENCE);
      }
      if (lid == 0) {
        R[wgid] = res_loc[0];
      }
    }
    // \cond
);
// \endcond

/**
 * See the docs for \link kernels/matrix_multiply.hpp
 * row_vector_matrix_multiply() \endlink
 */
const kernel_cl<in_buffer, in_buffer, out_buffer, int, int, TriangularViewCL,
                TriangularViewCL>
    row_vector_matrix_multiply("row_vector_matrix_multiply",
                               row_vector_matrix_multiply_kernel_code,
                               {{"LOCAL_SIZE_", 64},
                                {"REDUCTION_STEP_SIZE", 4}});

}  // namespace opencl_kernels
}  // namespace math
}  // namespace stan
#endif
#endif
