#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
  const char *filename = argv[1];
  ifstream stream(filename);
  cout << "double passband[128] = {\n  ";
  cout << setprecision(15);
  for(size_t i=0; i!=128; ++i)
  {
    unsigned channelNo;
    double xx, xy, yx, yy;
    stream >> channelNo >> xx >> xy >> yx >> yy;
    cout << ((xy + yx) * 0.5);
    if(i != 127) cout << ", ";
    if(i%4 == 3 && i!=127)
      cout << "\n  ";
  }
  std::cout << "\n};\n";
}
