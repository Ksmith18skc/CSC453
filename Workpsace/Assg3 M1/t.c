int g(int x, int y) {
  int z, w;
  {
    z = x;
    {
      w = z;
      {
        println(z);
	z = y;
      }
      println(z);
    }
  }
}

int f(int u, int v) {
  int x, y;
  {
    x = u;
    g(x, v);
    {
      y = v;
      g(y, u);
    }
  }  
}

int main() {
  int x, y, z;
  {
    {
      x = 12345;
      y = 23456;
    }
    {
      f(x, y);
    }
  }
}