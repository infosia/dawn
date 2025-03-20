SKIP: FAILED

void set_matrix_scalar(inout float2x3 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[0];
      break;
    case 1:
      mat[1] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[1];
      break;
  }
}

void set_matrix_scalar_1(inout float2x4 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[0];
      break;
    case 1:
      mat[1] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[1];
      break;
  }
}

void set_matrix_scalar_2(inout float3x2 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xx == int2(0, 1)) ? val.xx : mat[0];
      break;
    case 1:
      mat[1] = (row.xx == int2(0, 1)) ? val.xx : mat[1];
      break;
    case 2:
      mat[2] = (row.xx == int2(0, 1)) ? val.xx : mat[2];
      break;
  }
}

void set_matrix_scalar_3(inout float3x3 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[0];
      break;
    case 1:
      mat[1] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[1];
      break;
    case 2:
      mat[2] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[2];
      break;
  }
}

void set_matrix_scalar_4(inout float3x4 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[0];
      break;
    case 1:
      mat[1] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[1];
      break;
    case 2:
      mat[2] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[2];
      break;
  }
}

void set_matrix_scalar_5(inout float4x2 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xx == int2(0, 1)) ? val.xx : mat[0];
      break;
    case 1:
      mat[1] = (row.xx == int2(0, 1)) ? val.xx : mat[1];
      break;
    case 2:
      mat[2] = (row.xx == int2(0, 1)) ? val.xx : mat[2];
      break;
    case 3:
      mat[3] = (row.xx == int2(0, 1)) ? val.xx : mat[3];
      break;
  }
}

void set_matrix_scalar_6(inout float4x3 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[0];
      break;
    case 1:
      mat[1] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[1];
      break;
    case 2:
      mat[2] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[2];
      break;
    case 3:
      mat[3] = (row.xxx == int3(0, 1, 2)) ? val.xxx : mat[3];
      break;
  }
}

void set_matrix_scalar_7(inout float4x4 mat, int col, int row, float val) {
  switch (col) {
    case 0:
      mat[0] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[0];
      break;
    case 1:
      mat[1] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[1];
      break;
    case 2:
      mat[2] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[2];
      break;
    case 3:
      mat[3] = (row.xxxx == int4(0, 1, 2, 3)) ? val.xxxx : mat[3];
      break;
  }
}

static int x_GLF_global_loop_count = 0;
static float4 x_GLF_color = float4(0.0f, 0.0f, 0.0f, 0.0f);

void main_1() {
  float2x3 m23 = float2x3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float2x4 m24 = float2x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float3x2 m32 = float3x2(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float3x3 m33 = float3x3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float3x4 m34 = float3x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float4x2 m42 = float4x2(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float4x3 m43 = float4x3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  float4x4 m44 = float4x4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  int i = 0;
  int i_1 = 0;
  int i_2 = 0;
  int i_3 = 0;
  int i_4 = 0;
  int i_5 = 0;
  int i_6 = 0;
  int i_7 = 0;
  int i_8 = 0;
  int i_9 = 0;
  int i_10 = 0;
  int i_11 = 0;
  int i_12 = 0;
  int i_13 = 0;
  int i_14 = 0;
  int i_15 = 0;
  int i_16 = 0;
  int i_17 = 0;
  int i_18 = 0;
  int i_19 = 0;
  int i_20 = 0;
  int i_21 = 0;
  int i_22 = 0;
  int i_23 = 0;
  int i_24 = 0;
  int i_25 = 0;
  int i_26 = 0;
  int i_27 = 0;
  int i_28 = 0;
  int i_29 = 0;
  int i_30 = 0;
  int i_31 = 0;
  int i_32 = 0;
  int i_33 = 0;
  int i_34 = 0;
  int i_35 = 0;
  int i_36 = 0;
  int i_37 = 0;
  float sum = 0.0f;
  int r = 0;
  x_GLF_global_loop_count = 0;
  m23 = float2x3((0.0f).xxx, (0.0f).xxx);
  m24 = float2x4((0.0f).xxxx, (0.0f).xxxx);
  m32 = float3x2((0.0f).xx, (0.0f).xx, (0.0f).xx);
  m33 = float3x3((0.0f).xxx, (0.0f).xxx, (0.0f).xxx);
  m34 = float3x4((0.0f).xxxx, (0.0f).xxxx, (0.0f).xxxx);
  m42 = float4x2((0.0f).xx, (0.0f).xx, (0.0f).xx, (0.0f).xx);
  m43 = float4x3((0.0f).xxx, (0.0f).xxx, (0.0f).xxx, (0.0f).xxx);
  m44 = float4x4((0.0f).xxxx, (0.0f).xxxx, (0.0f).xxxx, (0.0f).xxxx);
  i = 0;
  while (true) {
    if ((i < 1)) {
    } else {
      break;
    }
    i_1 = 0;
    while (true) {
      if ((i_1 < 1)) {
      } else {
        break;
      }
      i_2 = 0;
      while (true) {
        if ((i_2 < 1)) {
        } else {
          break;
        }
        i_3 = 0;
        while (true) {
          if ((i_3 < 1)) {
          } else {
            break;
          }
          i_4 = 0;
          while (true) {
            if ((i_4 < 1)) {
            } else {
              break;
            }
            i_5 = 0;
            while (true) {
              if ((i_5 < 1)) {
              } else {
                break;
              }
              i_6 = 0;
              while (true) {
                if ((i_6 < 1)) {
                } else {
                  break;
                }
                i_7 = 0;
                while (true) {
                  if ((i_7 < 1)) {
                  } else {
                    break;
                  }
                  i_8 = 0;
                  while (true) {
                    if ((i_8 < 1)) {
                    } else {
                      break;
                    }
                    i_9 = 0;
                    while (true) {
                      if ((i_9 < 1)) {
                      } else {
                        break;
                      }
                      i_10 = 0;
                      while (true) {
                        if ((i_10 < 1)) {
                        } else {
                          break;
                        }
                        i_11 = 0;
                        while (true) {
                          if ((i_11 < 1)) {
                          } else {
                            break;
                          }
                          i_12 = 0;
                          while (true) {
                            if ((i_12 < 1)) {
                            } else {
                              break;
                            }
                            i_13 = 0;
                            while (true) {
                              if ((i_13 < 1)) {
                              } else {
                                break;
                              }
                              i_14 = 0;
                              while (true) {
                                if ((i_14 < 1)) {
                                } else {
                                  break;
                                }
                                i_15 = 0;
                                while (true) {
                                  if ((i_15 < 1)) {
                                  } else {
                                    break;
                                  }
                                  i_16 = 0;
                                  while (true) {
                                    if ((i_16 < 1)) {
                                    } else {
                                      break;
                                    }
                                    i_17 = 0;
                                    while (true) {
                                      if ((i_17 < 1)) {
                                      } else {
                                        break;
                                      }
                                      i_18 = 0;
                                      while (true) {
                                        if ((i_18 < 1)) {
                                        } else {
                                          break;
                                        }
                                        i_19 = 0;
                                        while (true) {
                                          if ((i_19 < 1)) {
                                          } else {
                                            break;
                                          }
                                          i_20 = 0;
                                          while (true) {
                                            if ((i_20 < 1)) {
                                            } else {
                                              break;
                                            }
                                            i_21 = 0;
                                            while (true) {
                                              if ((i_21 < 1)) {
                                              } else {
                                                break;
                                              }
                                              i_22 = 0;
                                              while (true) {
                                                if ((i_22 < 1)) {
                                                } else {
                                                  break;
                                                }
                                                i_23 = 0;
                                                while (true) {
                                                  if ((i_23 < 1)) {
                                                  } else {
                                                    break;
                                                  }
                                                  i_24 = 0;
                                                  while (true) {
                                                    if ((i_24 < 1)) {
                                                    } else {
                                                      break;
                                                    }
                                                    i_25 = 0;
                                                    while (true) {
                                                      if ((i_25 < 1)) {
                                                      } else {
                                                        break;
                                                      }
                                                      i_26 = 0;
                                                      while (true) {
                                                        if ((i_26 < 1)) {
                                                        } else {
                                                          break;
                                                        }
                                                        i_27 = 0;
                                                        while (true) {
                                                          if ((i_27 < 1)) {
                                                          } else {
                                                            break;
                                                          }
                                                          i_28 = 0;
                                                          while (true) {
                                                            if ((i_28 < 1)) {
                                                            } else {
                                                              break;
                                                            }
                                                            i_29 = 0;
                                                            while (true) {
                                                              if ((i_29 < 1)) {
                                                              } else {
                                                                break;
                                                              }
                                                              i_30 = 0;
                                                              while (true) {
                                                                if ((i_30 < 1)) {
                                                                } else {
                                                                  break;
                                                                }
                                                                i_31 = 0;
                                                                while (true) {
                                                                  if ((i_31 < 1)) {
                                                                  } else {
                                                                    break;
                                                                  }
                                                                  i_32 = 0;
                                                                  while (true) {
                                                                    if ((i_32 < 1)) {
                                                                    } else {
                                                                      break;
                                                                    }
                                                                    i_33 = 0;
                                                                    while (true) {
                                                                      if ((i_33 < 1)) {
                                                                      } else {
                                                                        break;
                                                                      }
                                                                      i_34 = 0;
                                                                      while (true) {
                                                                        if ((i_34 < 1)) {
                                                                        } else {
                                                                          break;
                                                                        }
                                                                        i_35 = 0;
                                                                        while (true) {
                                                                          if ((i_35 < 1)) {
                                                                          } else {
                                                                            break;
                                                                          }
                                                                          i_36 = 0;
                                                                          while (true) {
                                                                            if ((i_36 < 1)) {
                                                                            } else {
                                                                              break;
                                                                            }
                                                                            i_37 = 0;
                                                                            while (true) {
                                                                              if ((i_37 < 1)) {
                                                                              } else {
                                                                                break;
                                                                              }
                                                                              while (true) {
                                                                                x_GLF_global_loop_count = (x_GLF_global_loop_count + 1);
                                                                                {
                                                                                  int x_373 = x_GLF_global_loop_count;
                                                                                  if (!((x_373 < 98))) { break; }
                                                                                }
                                                                              }
                                                                              int x_375 = i_37;
                                                                              int x_376 = i_37;
                                                                              set_matrix_scalar(m23, min(uint(x_375), 1u), min(uint(x_376), 2u), 1.0f);
                                                                              int x_378 = i_37;
                                                                              int x_379 = i_37;
                                                                              set_matrix_scalar_1(m24, min(uint(x_378), 1u), min(uint(x_379), 3u), 1.0f);
                                                                              int x_381 = i_37;
                                                                              int x_382 = i_37;
                                                                              set_matrix_scalar_2(m32, min(uint(x_381), 2u), min(uint(x_382), 1u), 1.0f);
                                                                              int x_384 = i_37;
                                                                              int x_385 = i_37;
                                                                              set_matrix_scalar_3(m33, min(uint(x_384), 2u), min(uint(x_385), 2u), 1.0f);
                                                                              int x_387 = i_37;
                                                                              int x_388 = i_37;
                                                                              set_matrix_scalar_4(m34, min(uint(x_387), 2u), min(uint(x_388), 3u), 1.0f);
                                                                              int x_390 = i_37;
                                                                              int x_391 = i_37;
                                                                              set_matrix_scalar_5(m42, min(uint(x_390), 3u), min(uint(x_391), 1u), 1.0f);
                                                                              int x_393 = i_37;
                                                                              int x_394 = i_37;
                                                                              set_matrix_scalar_6(m43, min(uint(x_393), 3u), min(uint(x_394), 2u), 1.0f);
                                                                              int x_396 = i_37;
                                                                              int x_397 = i_37;
                                                                              set_matrix_scalar_7(m44, min(uint(x_396), 3u), min(uint(x_397), 3u), 1.0f);
                                                                              {
                                                                                i_37 = (i_37 + 1);
                                                                              }
                                                                            }
                                                                            {
                                                                              i_36 = (i_36 + 1);
                                                                            }
                                                                          }
                                                                          {
                                                                            i_35 = (i_35 + 1);
                                                                          }
                                                                        }
                                                                        {
                                                                          i_34 = (i_34 + 1);
                                                                        }
                                                                      }
                                                                      {
                                                                        i_33 = (i_33 + 1);
                                                                      }
                                                                    }
                                                                    {
                                                                      i_32 = (i_32 + 1);
                                                                    }
                                                                  }
                                                                  {
                                                                    i_31 = (i_31 + 1);
                                                                  }
                                                                }
                                                                {
                                                                  i_30 = (i_30 + 1);
                                                                }
                                                              }
                                                              {
                                                                i_29 = (i_29 + 1);
                                                              }
                                                            }
                                                            {
                                                              i_28 = (i_28 + 1);
                                                            }
                                                          }
                                                          {
                                                            i_27 = (i_27 + 1);
                                                          }
                                                        }
                                                        {
                                                          i_26 = (i_26 + 1);
                                                        }
                                                      }
                                                      {
                                                        i_25 = (i_25 + 1);
                                                      }
                                                    }
                                                    {
                                                      i_24 = (i_24 + 1);
                                                    }
                                                  }
                                                  {
                                                    i_23 = (i_23 + 1);
                                                  }
                                                }
                                                {
                                                  i_22 = (i_22 + 1);
                                                }
                                              }
                                              {
                                                i_21 = (i_21 + 1);
                                              }
                                            }
                                            {
                                              i_20 = (i_20 + 1);
                                            }
                                          }
                                          {
                                            i_19 = (i_19 + 1);
                                          }
                                        }
                                        {
                                          i_18 = (i_18 + 1);
                                        }
                                      }
                                      {
                                        i_17 = (i_17 + 1);
                                      }
                                    }
                                    {
                                      i_16 = (i_16 + 1);
                                    }
                                  }
                                  {
                                    i_15 = (i_15 + 1);
                                  }
                                }
                                {
                                  i_14 = (i_14 + 1);
                                }
                              }
                              {
                                i_13 = (i_13 + 1);
                              }
                            }
                            {
                              i_12 = (i_12 + 1);
                            }
                          }
                          {
                            i_11 = (i_11 + 1);
                          }
                        }
                        {
                          i_10 = (i_10 + 1);
                        }
                      }
                      {
                        i_9 = (i_9 + 1);
                      }
                    }
                    {
                      i_8 = (i_8 + 1);
                    }
                  }
                  {
                    i_7 = (i_7 + 1);
                  }
                }
                {
                  i_6 = (i_6 + 1);
                }
              }
              {
                i_5 = (i_5 + 1);
              }
            }
            {
              i_4 = (i_4 + 1);
            }
          }
          {
            i_3 = (i_3 + 1);
          }
        }
        {
          i_2 = (i_2 + 1);
        }
      }
      {
        i_1 = (i_1 + 1);
      }
    }
    {
      i = (i + 1);
    }
  }
  sum = 0.0f;
  r = 0;
  while (true) {
    if ((x_GLF_global_loop_count < 100)) {
    } else {
      break;
    }
    x_GLF_global_loop_count = (x_GLF_global_loop_count + 1);
    sum = (sum + m23[0][min(uint(r), 2u)]);
    sum = (sum + m24[0][min(uint(r), 3u)]);
    sum = (sum + m32[0][min(uint(r), 1u)]);
    sum = (sum + m33[0][min(uint(r), 2u)]);
    sum = (sum + m34[0][min(uint(r), 3u)]);
    sum = (sum + m42[0][min(uint(r), 1u)]);
    sum = (sum + m43[0][min(uint(r), 2u)]);
    sum = (sum + m44[0][min(uint(r), 3u)]);
    {
      r = (r + 1);
    }
  }
  if ((sum == 8.0f)) {
    x_GLF_color = float4(1.0f, 0.0f, 0.0f, 1.0f);
  } else {
    x_GLF_color = (0.0f).xxxx;
  }
  return;
}

struct main_out {
  float4 x_GLF_color_1;
};
struct tint_symbol {
  float4 x_GLF_color_1 : SV_Target0;
};

main_out main_inner() {
  main_1();
  main_out tint_symbol_1 = {x_GLF_color};
  return tint_symbol_1;
}

tint_symbol main() {
  main_out inner_result = main_inner();
  tint_symbol wrapper_result = (tint_symbol)0;
  wrapper_result.x_GLF_color_1 = inner_result.x_GLF_color_1;
  return wrapper_result;
}
