
[numthreads(1, 1, 1)]
void main() {
  bool a = true;
  bool v_1 = (a & true);
  bool v_2 = true;
  bool v = ((false) ? (v_2) : (v_1));
}

