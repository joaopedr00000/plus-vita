// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_THEME_PRESETS_H
#define PV_THEME_PRESETS_H
#include "custom_theme.h"
#define PV_THEME_COUNT 50u
#define PV_THEME_SLOTS 3u
#define PV_THEME_PAGE 5u
#define PV_THEME_RETENTION_SECONDS (10u*24u*60u*60u)
#define PV_THEME_ROTATION_US UINT64_C(60000000)
typedef struct { const char *name; uint32_t primary,secondary; } PvThemePreset;
/* Stable 1-based IDs; 0 is the original gold design. */
static const PvThemePreset pv_presets[PV_THEME_COUNT+1]={
    {"Original VitaPlus",PV_PRIMARY_DEFAULT,PV_SECONDARY_DEFAULT},
    {"GTA VI - Vice City",0xff4d9du,0x8b5cf6u},
    {"GTA V",0x479c40u,0xd9d9d9u},
    {"Red Dead Redemption 2",0xdb2020u,0x171717u},
    {"Cyberpunk 2077",0xfcee09u,0x00d9ffu},
    {"The Legend of Zelda",0x347c45u,0xd4af37u},
    {"Super Mario",0xe52521u,0x049cd8u},
    {"Pokemon",0xffcb05u,0x3466afu},
    {"Fortnite",0x9b51e0u,0x00caffu},
    {"Minecraft",0x65a743u,0x805536u},
    {"Call of Duty",0xa3ac65u,0x292929u},
    {"Resident Evil",0xa80018u,0x111111u},
    {"God of War - Greek Era",0xc52d29u,0xc4c8cbu},
    {"Ghost of Yotei",0xe9c553u,0x363636u},
    {"Horizon",0xed8738u,0x2895cau},
    {"Elden Ring",0xc5a355u,0x344f45u},
    {"Sonic",0x0058e9u,0xffd700u},
    {"Dragon Ball",0xff8c00u,0x2459bbu},
    {"Naruto",0xff7900u,0x222222u},
    {"One Piece",0xd62828u,0xf6c343u},
    {"Demon Slayer",0x20986eu,0x151515u},
    {"Jujutsu Kaisen",0x6954e8u,0x00bcffu},
    {"Chainsaw Man",0xe77c23u,0x8b1818u},
    {"Solo Leveling",0x944dffu,0x111111u},
    {"Attack on Titan",0x647b4cu,0x8b6345u},
    {"Black Clover",0xb82d35u,0x171717u},
    {"JoJo - Steel Ball Run",0xcfb044u,0x9152afu},
    {"SpongeBob SquarePants",0xffe52bu,0x40d8ddu},
    {"Rick and Morty",0x82ea39u,0x7ad9ffu},
    {"The Simpsons",0xffd90fu,0x62c9f4u},
    {"Adventure Time",0x48b6eeu,0xf8be39u},
    {"KPop Demon Hunters",0xf057d9u,0x7742ffu},
    {"Toy Story",0x52b3edu,0x8bc34au},
    {"Sony PlayStation",0x003791u,0xffffffu},
    {"PS Vita Classic",0x24a8ffu,0x062459u},
    {"PlayStation 2",0x394af4u,0x101010u},
    {"Nintendo Switch",0xff4545u,0x00b9e8u},
    {"Xbox",0x107c10u,0x151515u},
    {"Linux - Tux",0xffc928u,0x202020u},
    {"Ubuntu",0xe95420u,0x77216fu},
    {"Steam Deck",0x66c0f4u,0x171a21u},
    {"Rock 'n' Roll",0xd0d0d0u,0xa4161au},
    {"Metallica - Snakebyte",0xc0c0c0u,0x171717u},
    {"Iron Maiden",0xcb2027u,0x333333u},
    {"Black Sabbath",0x8149bdu,0x111111u},
    {"Slipknot",0xa90016u,0x191919u},
    {"Kawasaki Ninja H2R",0x69be28u,0x202020u},
    {"Yamaha R1",0x1746c7u,0xc7c7c7u},
    {"Ducati Panigale V4",0xcc0000u,0x111111u},
    {"Harley-Davidson",0xff6600u,0x151515u},
    {"Honda CBR1000RR-R",0xe00024u,0x234cc4u},
};
static inline unsigned pv_theme_id(unsigned id){return id<=PV_THEME_COUNT?id:0;}
static inline int pv_theme_logo_index(unsigned id){return id==1?0:id==12?1:id==42?2:-1;}
static inline bool pv_theme_expired(uint64_t now,uint64_t last,unsigned id,unsigned active){
    return id<=PV_THEME_COUNT&&id!=active&&last>=UINT64_C(946684800)&&now>=last&&now-last>=PV_THEME_RETENTION_SECONDS;
}
#endif
