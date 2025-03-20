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
    int x_105 = i;
    if ((x_105 < 1)) {
    } else {
      break;
    }
    i_1 = 0;
    while (true) {
      int x_112 = i_1;
      if ((x_112 < 1)) {
      } else {
        break;
      }
      i_2 = 0;
      while (true) {
        int x_119 = i_2;
        if ((x_119 < 1)) {
        } else {
          break;
        }
        i_3 = 0;
        while (true) {
          int x_126 = i_3;
          if ((x_126 < 1)) {
          } else {
            break;
          }
          i_4 = 0;
          while (true) {
            int x_133 = i_4;
            if ((x_133 < 1)) {
            } else {
              break;
            }
            i_5 = 0;
            while (true) {
              int x_140 = i_5;
              if ((x_140 < 1)) {
              } else {
                break;
              }
              i_6 = 0;
              while (true) {
                int x_147 = i_6;
                if ((x_147 < 1)) {
                } else {
                  break;
                }
                i_7 = 0;
                while (true) {
                  int x_154 = i_7;
                  if ((x_154 < 1)) {
                  } else {
                    break;
                  }
                  i_8 = 0;
                  while (true) {
                    int x_161 = i_8;
                    if ((x_161 < 1)) {
                    } else {
                      break;
                    }
                    i_9 = 0;
                    while (true) {
                      int x_168 = i_9;
                      if ((x_168 < 1)) {
                      } else {
                        break;
                      }
                      i_10 = 0;
                      while (true) {
                        int x_175 = i_10;
                        if ((x_175 < 1)) {
                        } else {
                          break;
                        }
                        i_11 = 0;
                        while (true) {
                          int x_182 = i_11;
                          if ((x_182 < 1)) {
                          } else {
                            break;
                          }
                          i_12 = 0;
                          while (true) {
                            int x_189 = i_12;
                            if ((x_189 < 1)) {
                            } else {
                              break;
                            }
                            i_13 = 0;
                            while (true) {
                              int x_196 = i_13;
                              if ((x_196 < 1)) {
                              } else {
                                break;
                              }
                              i_14 = 0;
                              while (true) {
                                int x_203 = i_14;
                                if ((x_203 < 1)) {
                                } else {
                                  break;
                                }
                                i_15 = 0;
                                while (true) {
                                  int x_210 = i_15;
                                  if ((x_210 < 1)) {
                                  } else {
                                    break;
                                  }
                                  i_16 = 0;
                                  while (true) {
                                    int x_217 = i_16;
                                    if ((x_217 < 1)) {
                                    } else {
                                      break;
                                    }
                                    i_17 = 0;
                                    while (true) {
                                      int x_224 = i_17;
                                      if ((x_224 < 1)) {
                                      } else {
                                        break;
                                      }
                                      i_18 = 0;
                                      while (true) {
                                        int x_231 = i_18;
                                        if ((x_231 < 1)) {
                                        } else {
                                          break;
                                        }
                                        i_19 = 0;
                                        while (true) {
                                          int x_238 = i_19;
                                          if ((x_238 < 1)) {
                                          } else {
                                            break;
                                          }
                                          i_20 = 0;
                                          while (true) {
                                            int x_245 = i_20;
                                            if ((x_245 < 1)) {
                                            } else {
                                              break;
                                            }
                                            i_21 = 0;
                                            while (true) {
                                              int x_252 = i_21;
                                              if ((x_252 < 1)) {
                                              } else {
                                                break;
                                              }
                                              i_22 = 0;
                                              while (true) {
                                                int x_259 = i_22;
                                                if ((x_259 < 1)) {
                                                } else {
                                                  break;
                                                }
                                                i_23 = 0;
                                                while (true) {
                                                  int x_266 = i_23;
                                                  if ((x_266 < 1)) {
                                                  } else {
                                                    break;
                                                  }
                                                  i_24 = 0;
                                                  while (true) {
                                                    int x_273 = i_24;
                                                    if ((x_273 < 1)) {
                                                    } else {
                                                      break;
                                                    }
                                                    i_25 = 0;
                                                    while (true) {
                                                      int x_280 = i_25;
                                                      if ((x_280 < 1)) {
                                                      } else {
                                                        break;
                                                      }
                                                      i_26 = 0;
                                                      while (true) {
                                                        int x_287 = i_26;
                                                        if ((x_287 < 1)) {
                                                        } else {
                                                          break;
                                                        }
                                                        i_27 = 0;
                                                        while (true) {
                                                          int x_294 = i_27;
                                                          if ((x_294 < 1)) {
                                                          } else {
                                                            break;
                                                          }
                                                          i_28 = 0;
                                                          while (true) {
                                                            int x_301 = i_28;
                                                            if ((x_301 < 1)) {
                                                            } else {
                                                              break;
                                                            }
                                                            i_29 = 0;
                                                            while (true) {
                                                              int x_308 = i_29;
                                                              if ((x_308 < 1)) {
                                                              } else {
                                                                break;
                                                              }
                                                              i_30 = 0;
                                                              while (true) {
                                                                int x_315 = i_30;
                                                                if ((x_315 < 1)) {
                                                                } else {
                                                                  break;
                                                                }
                                                                i_31 = 0;
                                                                while (true) {
                                                                  int x_322 = i_31;
                                                                  if ((x_322 < 1)) {
                                                                  } else {
                                                                    break;
                                                                  }
                                                                  i_32 = 0;
                                                                  while (true) {
                                                                    int x_329 = i_32;
                                                                    if ((x_329 < 1)) {
                                                                    } else {
                                                                      break;
                                                                    }
                                                                    i_33 = 0;
                                                                    while (true) {
                                                                      int x_336 = i_33;
                                                                      if ((x_336 < 1)) {
                                                                      } else {
                                                                        break;
                                                                      }
                                                                      i_34 = 0;
                                                                      while (true) {
                                                                        int x_343 = i_34;
                                                                        if ((x_343 < 1)) {
                                                                        } else {
                                                                          break;
                                                                        }
                                                                        i_35 = 0;
                                                                        while (true) {
                                                                          int x_350 = i_35;
                                                                          if ((x_350 < 1)) {
                                                                          } else {
                                                                            break;
                                                                          }
                                                                          i_36 = 0;
                                                                          while (true) {
                                                                            int x_357 = i_36;
                                                                            if ((x_357 < 1)) {
                                                                            } else {
                                                                              break;
                                                                            }
                                                                            i_37 = 0;
                                                                            while (true) {
                                                                              int x_364 = i_37;
                                                                              if ((x_364 < 1)) {
                                                                              } else {
                                                                                break;
                                                                              }
                                                                              while (true) {
                                                                                int x_371 = x_GLF_global_loop_count;
                                                                                x_GLF_global_loop_count = (x_371 + 1);
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
                                                                                int x_399 = i_37;
                                                                                i_37 = (x_399 + 1);
                                                                              }
                                                                            }
                                                                            {
                                                                              int x_401 = i_36;
                                                                              i_36 = (x_401 + 1);
                                                                            }
                                                                          }
                                                                          {
                                                                            int x_403 = i_35;
                                                                            i_35 = (x_403 + 1);
                                                                          }
                                                                        }
                                                                        {
                                                                          int x_405 = i_34;
                                                                          i_34 = (x_405 + 1);
                                                                        }
                                                                      }
                                                                      {
                                                                        int x_407 = i_33;
                                                                        i_33 = (x_407 + 1);
                                                                      }
                                                                    }
                                                                    {
                                                                      int x_409 = i_32;
                                                                      i_32 = (x_409 + 1);
                                                                    }
                                                                  }
                                                                  {
                                                                    int x_411 = i_31;
                                                                    i_31 = (x_411 + 1);
                                                                  }
                                                                }
                                                                {
                                                                  int x_413 = i_30;
                                                                  i_30 = (x_413 + 1);
                                                                }
                                                              }
                                                              {
                                                                int x_415 = i_29;
                                                                i_29 = (x_415 + 1);
                                                              }
                                                            }
                                                            {
                                                              int x_417 = i_28;
                                                              i_28 = (x_417 + 1);
                                                            }
                                                          }
                                                          {
                                                            int x_419 = i_27;
                                                            i_27 = (x_419 + 1);
                                                          }
                                                        }
                                                        {
                                                          int x_421 = i_26;
                                                          i_26 = (x_421 + 1);
                                                        }
                                                      }
                                                      {
                                                        int x_423 = i_25;
                                                        i_25 = (x_423 + 1);
                                                      }
                                                    }
                                                    {
                                                      int x_425 = i_24;
                                                      i_24 = (x_425 + 1);
                                                    }
                                                  }
                                                  {
                                                    int x_427 = i_23;
                                                    i_23 = (x_427 + 1);
                                                  }
                                                }
                                                {
                                                  int x_429 = i_22;
                                                  i_22 = (x_429 + 1);
                                                }
                                              }
                                              {
                                                int x_431 = i_21;
                                                i_21 = (x_431 + 1);
                                              }
                                            }
                                            {
                                              int x_433 = i_20;
                                              i_20 = (x_433 + 1);
                                            }
                                          }
                                          {
                                            int x_435 = i_19;
                                            i_19 = (x_435 + 1);
                                          }
                                        }
                                        {
                                          int x_437 = i_18;
                                          i_18 = (x_437 + 1);
                                        }
                                      }
                                      {
                                        int x_439 = i_17;
                                        i_17 = (x_439 + 1);
                                      }
                                    }
                                    {
                                      int x_441 = i_16;
                                      i_16 = (x_441 + 1);
                                    }
                                  }
                                  {
                                    int x_443 = i_15;
                                    i_15 = (x_443 + 1);
                                  }
                                }
                                {
                                  int x_445 = i_14;
                                  i_14 = (x_445 + 1);
                                }
                              }
                              {
                                int x_447 = i_13;
                                i_13 = (x_447 + 1);
                              }
                            }
                            {
                              int x_449 = i_12;
                              i_12 = (x_449 + 1);
                            }
                          }
                          {
                            int x_451 = i_11;
                            i_11 = (x_451 + 1);
                          }
                        }
                        {
                          int x_453 = i_10;
                          i_10 = (x_453 + 1);
                        }
                      }
                      {
                        int x_455 = i_9;
                        i_9 = (x_455 + 1);
                      }
                    }
                    {
                      int x_457 = i_8;
                      i_8 = (x_457 + 1);
                    }
                  }
                  {
                    int x_459 = i_7;
                    i_7 = (x_459 + 1);
                  }
                }
                {
                  int x_461 = i_6;
                  i_6 = (x_461 + 1);
                }
              }
              {
                int x_463 = i_5;
                i_5 = (x_463 + 1);
              }
            }
            {
              int x_465 = i_4;
              i_4 = (x_465 + 1);
            }
          }
          {
            int x_467 = i_3;
            i_3 = (x_467 + 1);
          }
        }
        {
          int x_469 = i_2;
          i_2 = (x_469 + 1);
        }
      }
      {
        int x_471 = i_1;
        i_1 = (x_471 + 1);
      }
    }
    {
      int x_473 = i;
      i = (x_473 + 1);
    }
  }
  sum = 0.0f;
  r = 0;
  while (true) {
    int x_479 = x_GLF_global_loop_count;
    if ((x_479 < 100)) {
    } else {
      break;
    }
    int x_482 = x_GLF_global_loop_count;
    x_GLF_global_loop_count = (x_482 + 1);
    int x_484 = r;
    float x_486 = m23[0][min(uint(x_484), 2u)];
    float x_487 = sum;
    sum = (x_487 + x_486);
    int x_489 = r;
    float x_491 = m24[0][min(uint(x_489), 3u)];
    float x_492 = sum;
    sum = (x_492 + x_491);
    int x_494 = r;
    float x_496 = m32[0][min(uint(x_494), 1u)];
    float x_497 = sum;
    sum = (x_497 + x_496);
    int x_499 = r;
    float x_501 = m33[0][min(uint(x_499), 2u)];
    float x_502 = sum;
    sum = (x_502 + x_501);
    int x_504 = r;
    float x_506 = m34[0][min(uint(x_504), 3u)];
    float x_507 = sum;
    sum = (x_507 + x_506);
    int x_509 = r;
    float x_511 = m42[0][min(uint(x_509), 1u)];
    float x_512 = sum;
    sum = (x_512 + x_511);
    int x_514 = r;
    float x_516 = m43[0][min(uint(x_514), 2u)];
    float x_517 = sum;
    sum = (x_517 + x_516);
    int x_519 = r;
    float x_521 = m44[0][min(uint(x_519), 3u)];
    float x_522 = sum;
    sum = (x_522 + x_521);
    {
      int x_524 = r;
      r = (x_524 + 1);
    }
  }
  float x_526 = sum;
  if ((x_526 == 8.0f)) {
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
