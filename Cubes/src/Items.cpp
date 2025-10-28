#include "Items.h"

static ItemInfo itemInfoTable[] = {
	{BlockType::btAir, uiNone},
	{BlockType::btGround, uiGroundBlock},
	{BlockType::btStone, uiStoneBlock},
	{BlockType::btSnow, uiSnowBlock},
	{BlockType::btIronOre, uiIronOreBlock},
};

static BlockInfo blockInfoTable[] = {
	{ItemType::None, TextureID::tidAir},
	{ItemType::GroundBlock, TextureID::tidGround},
	{ItemType::StoneBlock, TextureID::tidStone},
	{ItemType::SnowBlock, TextureID::tidSnow},
	{ItemType::IronOreBlock, TextureID::tidIronOre},
};

const ItemInfo* GetItemInfo(ItemType item)
{
	return &itemInfoTable[(int)item];
}

const BlockInfo* GetBlockInfo(BlockType item)
{
	return &blockInfoTable[(int)item];
}
