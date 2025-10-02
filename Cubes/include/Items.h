#pragma once
#include <glm.hpp>
#include "Typedefs.h"

enum class BlockType : u16 {
	btAir = 0,
	btGround,
	btStone,
	btSnow,
	btIronOre,
	texSun,
	texMoon,
	btCOUNT
};

enum class ItemType : u16 {
	None = 0,
	GroundBlock,
	StoneBlock,
	SnowBlock,
	IronOreBlock,
	COUNT
};

enum uiElemType : u16 {
	uiInventorySelectCell,
	uiInventoryCell,
	uiHeart,
	uiCross,
	uiGroundBlock,
	uiStoneBlock,
	uiSnowBlock,
	uiIronOreBlock,
	uiNone,
	uiCOUNT
};

struct ItemInfo {
	BlockType blockType;
	uiElemType uiElement;
};

struct BlockInfo {
	ItemType itemType;
};

struct Item {
	glm::vec3 pos;
	u32 count;
	ItemType type;
};

const ItemInfo* GetItemInfo(ItemType item);
const BlockInfo* GetBlockInfo(BlockType item);