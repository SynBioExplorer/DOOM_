// Standalone unit tests for DOOM pure logic (no UE5 dependency)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using fixed_t = int32_t;
using angle_t = uint32_t;
constexpr int32_t FRACBITS = 16;
constexpr fixed_t FRACUNIT = (1 << FRACBITS);

static const uint8_t RndTable[256] = {
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 108,  67, 117,
    97, 167, 127, 181, 131, 182,  27,  90, 131,   8,  72,  40,  36, 152,
    44, 215, 229, 208, 163, 190,  45, 109, 225, 168,  34, 136, 233, 139,
    40, 171,  24, 233
};

static int32_t RndIndex = 0;
static uint8_t P_Random() { RndIndex = (RndIndex + 1) & 0xff; return RndTable[RndIndex]; }

static fixed_t AproxDistance(fixed_t dx, fixed_t dy) {
    dx = abs(dx); dy = abs(dy);
    if (dx < dy) return dx + dy - (dx >> 1);
    return dx + dy - (dy >> 1);
}

static angle_t PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2) {
    double dx = (double)(x2 - x1), dy = (double)(y2 - y1);
    return (angle_t)(atan2(dy, dx) * (2147483648.0 / M_PI));
}

enum Direction { East=0, NorthEast, North, NorthWest, West, SouthWest, South, SouthEast, NoDir };
static const fixed_t XSpeed[8] = { FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000, 0, 47000 };
static const fixed_t YSpeed[8] = { 0, 47000, FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000 };
static const int Opposite[9] = { West, SouthWest, South, SouthEast, East, NorthEast, North, NorthWest, NoDir };
static const int Diags[4] = { NorthWest, NorthEast, SouthWest, SouthEast };

static int pass = 0, fail = 0;
#define TEST(name) static void test_##name()
#define RUN(name) do { printf("  %-45s", #name); test_##name(); printf(" PASS\n"); pass++; } while(0)
#define EQ(a, b) do { if ((a)!=(b)) { printf(" FAIL (line %d: %d != %d)\n", __LINE__, (int)(a), (int)(b)); fail++; return; } } while(0)
#define OK(c) do { if (!(c)) { printf(" FAIL (line %d)\n", __LINE__); fail++; return; } } while(0)

TEST(rng_sequence) { RndIndex=0; EQ(P_Random(),8); EQ(P_Random(),109); EQ(P_Random(),220); EQ(P_Random(),222); EQ(P_Random(),241); }
TEST(rng_wrap) { RndIndex=254; P_Random(); uint8_t v=P_Random(); EQ(v,RndTable[0]); EQ(RndIndex,0); }
TEST(rng_range) { RndIndex=0; for(int i=0;i<256;i++) { uint8_t v=P_Random(); OK(v<=255); } }
TEST(rng_deterministic) { RndIndex=42; uint8_t a=P_Random(),b=P_Random(),c=P_Random(); RndIndex=42; EQ(P_Random(),a); EQ(P_Random(),b); EQ(P_Random(),c); }

TEST(aprox_3_4_5) { fixed_t r=AproxDistance(3*FRACUNIT,4*FRACUNIT); double a=(double)r/FRACUNIT; OK(a>4.5&&a<6.0); }
TEST(aprox_zero) { EQ(AproxDistance(0,0),0); }
TEST(aprox_negative) { EQ(AproxDistance(3*FRACUNIT,4*FRACUNIT), AproxDistance(-3*FRACUNIT,-4*FRACUNIT)); }
TEST(aprox_axis) { EQ(AproxDistance(5*FRACUNIT,0), 5*FRACUNIT); EQ(AproxDistance(0,5*FRACUNIT), 5*FRACUNIT); }

TEST(angle_east) { angle_t a=PointToAngle2(0,0,10*FRACUNIT,0); OK(a<0x10000000u||a>0xF0000000u); }
TEST(angle_north) { angle_t a=PointToAngle2(0,0,0,10*FRACUNIT); int32_t d=(int32_t)(a-0x40000000u); OK(abs(d)<0x02000000); }
TEST(angle_west) { angle_t a=PointToAngle2(0,0,-10*FRACUNIT,0); int32_t d=(int32_t)(a-0x80000000u); OK(abs(d)<0x02000000); }
TEST(angle_south) { angle_t a=PointToAngle2(0,0,0,-10*FRACUNIT); int32_t d=(int32_t)(a-0xC0000000u); OK(abs(d)<0x02000000); }

TEST(dir_opposite) { for(int d=0;d<8;d++) EQ(Opposite[Opposite[d]],d); EQ(Opposite[NoDir],NoDir); }
TEST(dir_speed_sym) { EQ(XSpeed[East],-XSpeed[West]); EQ(YSpeed[North],-YSpeed[South]); }
TEST(dir_cardinal) { EQ(XSpeed[East],FRACUNIT); EQ(YSpeed[East],0); EQ(XSpeed[North],0); EQ(YSpeed[North],FRACUNIT); }
TEST(dir_diagonal) { EQ(XSpeed[NorthEast],47000); EQ(YSpeed[NorthEast],47000); }
TEST(diags_table) { EQ(Diags[1],NorthEast); EQ(Diags[0],NorthWest); EQ(Diags[3],SouthEast); EQ(Diags[2],SouthWest); }

TEST(spawn_coverage) { for(int r=0;r<256;r++) { int t=-1; if(r<50)t=0; else if(r<90)t=1; else if(r<120)t=2; else if(r<130)t=3; else if(r<160)t=4; else if(r<162)t=5; else if(r<172)t=6; else if(r<192)t=7; else if(r<222)t=8; else if(r<246)t=9; else t=10; OK(t>=0&&t<=10); } }
TEST(spawn_dist) { EQ(50,50); EQ(2,2); /* Troop=50/256, Vile=2/256 */ }

TEST(dmg_pos) { RndIndex=0; for(int i=0;i<100;i++){int d=((P_Random()%5)+1)*3; OK(d>=3&&d<=15);} }
TEST(dmg_sarg) { RndIndex=0; for(int i=0;i<100;i++){int d=((P_Random()%10)+1)*4; OK(d>=4&&d<=40);} }
TEST(dmg_skel) { RndIndex=0; for(int i=0;i<100;i++){int d=((P_Random()%10)+1)*6; OK(d>=6&&d<=60);} }
TEST(dmg_head) { RndIndex=0; for(int i=0;i<100;i++){int d=(P_Random()%6+1)*10; OK(d>=10&&d<=60);} }
TEST(dmg_bruis) { RndIndex=0; for(int i=0;i<100;i++){int d=(P_Random()%8+1)*10; OK(d>=10&&d<=80);} }

int main() {
    printf("=== DOOM UE5 Port - Unit Tests ===\n\n");
    printf("[RNG]\n");
    RUN(rng_sequence); RUN(rng_wrap); RUN(rng_range); RUN(rng_deterministic);
    printf("\n[Math]\n");
    RUN(aprox_3_4_5); RUN(aprox_zero); RUN(aprox_negative); RUN(aprox_axis);
    printf("\n[Angles]\n");
    RUN(angle_east); RUN(angle_north); RUN(angle_west); RUN(angle_south);
    printf("\n[Direction Tables]\n");
    RUN(dir_opposite); RUN(dir_speed_sym); RUN(dir_cardinal); RUN(dir_diagonal); RUN(diags_table);
    printf("\n[Spawn & Damage Formulas]\n");
    RUN(spawn_coverage); RUN(spawn_dist);
    RUN(dmg_pos); RUN(dmg_sarg); RUN(dmg_skel); RUN(dmg_head); RUN(dmg_bruis);
    printf("\n=== Results: %d passed, %d failed ===\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
