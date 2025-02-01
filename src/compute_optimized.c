#include <omp.h>
#include <x86intrin.h>

#include "compute.h"

// Computes the dot product of vec1 and vec2, both of size n
int dot(uint32_t n, int32_t *vec1, int32_t *vec2)
{
  // TODO: implement dot product of vec1 and vec2, both of size n
  int sum = 0;
  __m256i v1, v2, mul;
  __m256i acc = _mm256_setzero_si256();
  int tmp_arr[8];
  for (unsigned int i = 0; i < n / 8 * 8; i += 8)
  {
    v1 = _mm256_loadu_si256((__m256i *)(vec1 + i));
    v2 = _mm256_loadu_si256((__m256i *)(vec2 + i));
    mul = _mm256_mullo_epi32(v1, v2);
    acc = _mm256_add_epi32(acc, mul); // Accumulate results
  }
  _mm256_storeu_si256((__m256i *)tmp_arr, acc);
  sum = tmp_arr[0] + tmp_arr[1] + tmp_arr[2] + tmp_arr[3] + tmp_arr[4] + tmp_arr[5] + tmp_arr[6] + tmp_arr[7];
  for (unsigned int i = n / 8 * 8; i < n; i++)
  {
    sum += vec1[i] * vec2[i];
  }

  return sum;
}

// Computes the convolution of two matrices
int convolve(matrix_t *a_matrix, matrix_t *b_matrix, matrix_t **output_matrix)
{
  // TODO: convolve matrix a and matrix b, and store the resulting matrix in
  // output_matrix

  // flip matrix b
  int32_t *b_data = (int32_t *)malloc(sizeof(int32_t) * b_matrix->rows * b_matrix->cols);
  unsigned int b_size = b_matrix->rows * b_matrix->cols;
  {
    for (unsigned int i = 0; i < b_size; i++)
    {
      b_data[i] = b_matrix->data[b_size - 1 - i];
    }
  }

  *output_matrix = (matrix_t *)malloc(sizeof(matrix_t));
  (*output_matrix)->rows = a_matrix->rows - b_matrix->rows + 1;
  (*output_matrix)->cols = a_matrix->cols - b_matrix->cols + 1;
  (*output_matrix)->data = (int32_t *)malloc(sizeof(int32_t) * ((*output_matrix)->rows) * ((*output_matrix)->cols));
int32_t tmp_sum = 0;
  #pragma omp parallel for reduction(+:tmp_sum)
for (int i = 0; i < (*output_matrix)->rows; i++)
{
  for (int j = 0; j < (*output_matrix)->cols; j++)
  {
    tmp_sum = 0;
    for (int k = 0; k < b_matrix->rows; k++)
    {
      tmp_sum += dot(b_matrix->cols, a_matrix->data + (i + k) * a_matrix->cols + j, b_data + k * b_matrix->cols);
    }
    (*output_matrix)->data[i * (*output_matrix)->cols + j] = tmp_sum;
  }
}

  free(b_data);
  return 0;
}

// Executes a task
int execute_task(task_t *task)
{
  matrix_t *a_matrix, *b_matrix, *output_matrix;

  char *a_matrix_path = get_a_matrix_path(task);
  if (read_matrix(a_matrix_path, &a_matrix))
  {
    printf("Error reading matrix from %s\n", a_matrix_path);
    return -1;
  }
  free(a_matrix_path);

  char *b_matrix_path = get_b_matrix_path(task);
  if (read_matrix(b_matrix_path, &b_matrix))
  {
    printf("Error reading matrix from %s\n", b_matrix_path);
    return -1;
  }
  free(b_matrix_path);

  if (convolve(a_matrix, b_matrix, &output_matrix))
  {
    printf("convolve returned a non-zero integer\n");
    return -1;
  }

  char *output_matrix_path = get_output_matrix_path(task);
  if (write_matrix(output_matrix_path, output_matrix))
  {
    printf("Error writing matrix to %s\n", output_matrix_path);
    return -1;
  }
  free(output_matrix_path);

  free(a_matrix->data);
  free(b_matrix->data);
  free(output_matrix->data);
  free(a_matrix);
  free(b_matrix);
  free(output_matrix);
  return 0;
}
