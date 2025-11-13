#include "Items.h"

ItemInfo itemInfoTable[(int)ItemType::COUNT] = {
	{"None",			ItemType::None,			BlockType::btAir,		uiNone,			1},
	{"Ground block",	ItemType::GroundBlock,	BlockType::btGround,	uiGroundBlock,	1},
	{"Stone block",		ItemType::StoneBlock,	BlockType::btStone,		uiStoneBlock,	1},
	{"Snow block",		ItemType::SnowBlock,	BlockType::btSnow,		uiSnowBlock,	1},
	{"Iron ore block",	ItemType::IronOreBlock, BlockType::btIronOre,	uiIronOreBlock, 1},
	{"Wood block",		ItemType::WoodBlock,	BlockType::btWood,		uiWoodBlock,	1},
	{"Leaves block",	ItemType::LeavesBlock,	BlockType::btLeaves,	uiLeavesBlock,	1},
	
	{"Pickaxe",			ItemType::Pickaxe,		BlockType::btAir,		uiPickaxe,		5,
		true, {ItemType::WoodBlock, ItemType::WoodBlock, ItemType::WoodBlock}},
	{"Sword",			ItemType::Sword,		BlockType::btAir,		uiSword,		1,
		true, {ItemType::StoneBlock, ItemType::StoneBlock, ItemType::WoodBlock}},
};

BlockInfo blockInfoTable[(int)BlockType::btCOUNT] = {
	{.itemType = ItemType::None,			.textureID = TextureID::tidAir,		.hardness = 0},
	{.itemType = ItemType::GroundBlock,		.textureID = TextureID::tidGround,	.hardness = 0.5},
	{.itemType = ItemType::StoneBlock,		.textureID = TextureID::tidStone,	.hardness = 1},
	{.itemType = ItemType::SnowBlock,		.textureID = TextureID::tidSnow,	.hardness = 0.5},
	{.itemType = ItemType::IronOreBlock,	.textureID = TextureID::tidIronOre,	.hardness = 1},
	{.itemType = ItemType::WoodBlock,		.textureID = TextureID::tidWood,	.hardness = 1},
	{.itemType = ItemType::LeavesBlock,		.textureID = TextureID::tidLeaves,	.hardness = 0.25},
};

const ItemInfo* GetItemInfo(ItemType item)
{
	return &itemInfoTable[(int)item];
}

const BlockInfo* GetBlockInfo(BlockType item)
{
	return &blockInfoTable[(int)item];
}
