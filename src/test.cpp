#include <iostream>
#include <bitset>

void bitMask(uint8_t reg, uint8_t data, uint8_t pos, uint8_t len) 
{
    uint8_t mask = ((1 << len) -1) << pos;
    data <<= pos;
    data &= mask;
    reg &= ~(mask);
    reg |= data;
    std::bitset<8> x(reg);
    std::cout << x << std::endl;
}

int main(int argc, char const *argv[])
{
    uint8_t reg = 0x43;
    uint8_t data = 3;
    uint8_t pos = 0;
    uint8_t len = 3;

    //bitMask(reg, data, pos, len);
    uint8_t a = 0x01;
    std::bitset<8> A(a);
    uint8_t b = 0x02;
    std::bitset<8> B(b);
    uint8_t c;
    std::bitset<8> C(c);
    C = (A | B);
    uint8_t d = 0x07;
    std::bitset<8> D(d);
    uint8_t e;
    std::bitset<8> E(e); 
    E = (C & D) >> 2;

    std::cout << E << std::endl;

    return 0;
}
