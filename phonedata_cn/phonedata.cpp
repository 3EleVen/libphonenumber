#include <fstream>
#include <string>
#include "phonedata.h"
#include <cassert>
#include <algorithm>
#include <list>

static std::string CARD_NAME[LANG_NUM][CARD_NUM] = {
	"Unknown", "China Mobile", "China Unicom", "China Telecomm", "China Telecomm VNO", "China Unicom VNO", "China Mobile VNO",
	"未知", "中国移动", "中国联通", "中国电信", "中国电信虚拟运营商", "中国联通虚拟运营商", "中国移动虚拟运营商",
	"未知", "中國移動", "中國聯通", "中國電信", "中國電信 VNO", "中國聯通 VNO", "中國移動 VNO"
};

static std::vector<std::string> ssplit(const std::string &str, const std::string &c)
{
	std::vector<std::string> vec;
	std::string::size_type pos1 = 0, pos2 = str.find(c);
	while (std::string::npos != pos2)
	{
		std::string tmp = str.substr(pos1, pos2 - pos1);
		if (!tmp.empty())
		{
			vec.push_back(std::move(tmp));
		}

		pos1 = pos2 + c.size();
		pos2 = str.find(c, pos1);
	}
	if (pos1 < str.length())
	{
		vec.push_back(str.substr(pos1));
	}
	return vec;
}

std::string getPhoneType(CARDTYPE type, LANG lang)
{
	assert(type > UNKNOWN && type <= CMCC_V);
	assert(lang >= en && lang <= zh_Hant);
	
	return CARD_NAME[lang][type];
}

PhoneData::PhoneData()
{
	std::ifstream stream("phone.dat", std::ios::binary);
	if (!stream.is_open())
	{
		head = nullptr;
		return;
	}

	buffer = std::vector<char>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	head = reinterpret_cast<DataHead *>(buffer.data());
	recordCount = (buffer.size() - head->offset) / PHONE_INDEX_LENGTH;
}

PhoneInfo PhoneData::lookUp(int64_t phone) const
{
	if (phone >= 1000000 && phone <= 99999999999)
	{
		while (phone > 9999999)
		{
			phone /= 10;
		}
		return _lookUp(static_cast<uint32_t>(phone));
	}

	return PhoneInfo();
}

PhoneInfo PhoneData::lookUp(const std::string &phone) const
{
	if (phone.size() >= 7 && phone.size() <= 11)
	{
		return _lookUp(std::stoul(phone.substr(0, 7)));
	}

	return PhoneInfo();
}

PhoneInfo PhoneData::_lookUp(uint32_t phone7) const
{
	if (head && phone7 >= 1000000 && phone7 <= 99999999999)
	{
		size_t left = 0;
		size_t right = recordCount;

		while (left <= right)
		{
			size_t middle = (left + right) / 2;
			size_t currentOffset = head->offset + middle * PHONE_INDEX_LENGTH;
			if (currentOffset >= buffer.size())
			{
				return PhoneInfo();
			}

			auto _buffer = std::vector<char>(buffer.cbegin() + currentOffset, buffer.cbegin() + currentOffset + PHONE_INDEX_LENGTH);
			Record *_record = reinterpret_cast<Record *>(_buffer.data());

			if (_record->phone > phone7)
			{
				right = middle - 1;
			}
			else if (_record->phone < phone7)
			{
				left = middle + 1;
			}
			else
			{
				std::string recordContent = getRecordContent(buffer, _record->offset);
				std::vector <std::string> contents = ssplit(recordContent, std::string("|"));
				PhoneInfo info;
				info.type		= static_cast<CARDTYPE>(_record->type);
				info.phone		= _record->phone;
				info.province	= contents[PROVINCE];
				info.city		= contents[CITY];
				info.zipCode	= contents[ZIPCODE];
				info.areaCode	= contents[AREACODE];

				return info;
			}
		}
	}

	return PhoneInfo();
}

std::string PhoneData::getRecordContent(const std::vector<char> &buffer, size_t startOffset)
{
	size_t endOffset = std::find(buffer.cbegin() + startOffset, buffer.cend(), '\0') - buffer.cbegin();
	return std::string(buffer.cbegin() + startOffset, buffer.cbegin() + endOffset);
}

std::vector<PhoneInfo> PhoneData::loadAll() 
{
	std::vector<PhoneInfo> infoVec;
	if (head) {
		for (size_t cur = 0; cur < recordCount; cur++) {
			size_t currentOffset = head->offset + cur * PHONE_INDEX_LENGTH;
			
			auto _buffer = std::vector<char>(buffer.cbegin() + currentOffset, buffer.cbegin() + currentOffset + PHONE_INDEX_LENGTH);
			Record *_record = reinterpret_cast<Record *>(_buffer.data());

			std::string recordContent = getRecordContent(buffer, _record->offset);
			std::vector <std::string> contents = ssplit(recordContent, std::string("|"));
			PhoneInfo info;
			info.type		= static_cast<CARDTYPE>(_record->type);
			info.phone		= _record->phone;
			info.province	= contents[PROVINCE];
			info.city		= contents[CITY];
			info.zipCode	= contents[ZIPCODE];
			info.areaCode	= contents[AREACODE];
			infoVec.push_back(info);
		}
	}
	return infoVec;
}
