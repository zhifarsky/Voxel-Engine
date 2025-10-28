#pragma once
#include <glm.hpp>
#include "Typedefs.h"
#include "Renderer.h"

enum class BlockType : u8 {
	btAir = 0,
	btGround,
	btStone,
	btSnow,
	btIronOre,
	texSun,
	texMoon,
	btCOUNT
};

enum class ItemType : u8 {
	None = 0,
	GroundBlock,
	StoneBlock,
	SnowBlock,
	IronOreBlock,
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
	uiNone,
	uiCOUNT
};

struct ItemInfo {
	BlockType blockType;
	uiElemType uiElement;
};

struct BlockInfo {
	ItemType itemType;
	TextureID textureID;
};

struct Item {
	glm::vec3 pos;
	u32 count;
	ItemType type;
};

const ItemInfo* GetItemInfo(ItemType item);
const BlockInfo* GetBlockInfo(BlockType item);