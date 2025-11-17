#pragma once
#include <glm.hpp>
#include "Typedefs.h"
//#include "Renderer.h"

enum class BlockType : u8 {
	btAir = 0,
	btGround,
	btStone,
	btSnow,
	btIronOre,
	btWood,
	btLeaves,
	texSun,
	texMoon,
	btCOUNT
};

// в том же порядке, что и в атласе
enum TextureID : u8 {
	tidGround,
	tidStone,
	tidSun,
	tidMoon,
	tidSnow,
	tidIronOre,
	tidWood,
	tidLeaves,
	tidAir,
	tidCount
};

enum class ItemType : u8 {
	None = 0,
	GroundBlock,
	StoneBlock,
	SnowBlock,
	IronOreBlock,
	WoodBlock,
	LeavesBlock,
	
	Pickaxe,
	Sword,
	
	COUNT
};

enum uiElemType : u8 {
	uiInventorySelectCell,
	uiInventoryCell,
	uiHeart,
	uiCross,
	uiGroundBlock,
	uiStoneBlock,
	uiSnowBlock,
	uiIronOreBlock,
	uiSword,
	uiPickaxe,
	uiWoodBlock,
	uiLeavesBlock,
	uiNone,
	uiCOUNT
};

struct ItemInfo {
	const char* name;
	ItemType itemType;
	BlockType blockType;
	uiElemType uiElement;
	float blockDamage;
	bool craftable;
	ItemType craftScheme[9];
};

struct BlockInfo {
	ItemType itemType;
	TextureID textureID;
	float hardness;
};

struct Item {
	glm::vec3 pos;
	u32 count;
	ItemType type;
};

extern ItemInfo itemInfoTable[(int)ItemType::COUNT];
extern BlockInfo blockInfoTable[(int)BlockType::btCOUNT];
const ItemInfo* GetItemInfo(ItemType item);
const BlockInfo* GetBlockInfo(BlockType item);