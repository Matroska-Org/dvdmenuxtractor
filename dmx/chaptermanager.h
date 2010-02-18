#ifndef CHAPTER_MANAGER_H
#define CHAPTER_MANAGER_H

#include <QString>
#include "vobparser/IFOFile.h"

class QDomNode;
class QDomElement;
class QDomDocument;

class ChapterManager
{
public:
	ChapterManager(unsigned xmlIdentCount)
		: INDENT_COUNT(xmlIdentCount)
	{
	}
	bool generateScript(const IFOFile& ifoFile, const QString& filename, uint16_t title, const QString& editionUID) const;
	bool generateMenuScript(const IFOFile& ifoFile, const QString& filename, uint16_t title, const QString& editionUID) const;

	static QString INFO_SUFFIX;
	static QString CHAPTER_SUFFIX;

private:
	void generateInfoScript(const QString& filename, uint16_t title, const QString& editionUID) const;
	
	static QDomElement CreateDOMElement(QDomDocument& document, const QString& elementName, const QString& content);
	static void AddChapterTime(QDomDocument& document, QDomNode& parent, uint64_t start_time, uint64_t end_time);
	static void AddCodecPrivateData(QDomDocument& document, QDomNode& parent, const unsigned char *buffer, unsigned int size, bool createChapterProcessElement = true);
	static void AddPGCCommands(QDomDocument& document, QDomNode& parent, const pgc_command_tbl_t * command_tbl);
	static uint64_t AddProgram(QDomDocument& document, QDomNode& parent, const pgc_t *pgc, const QString& PgcUID, int program_number, const CellsListType& cell_list, int16_t pgc_num, const ttu_t *ptts, int title);
	static uint64_t AddPGC(QDomDocument& document, QDomNode& parent, const pgc_t * pgc, uint16_t pgc_num, unsigned char pgc_type, const CellsListType & cell_list, const ttu_t * ptts, int title);
	static QString GetPGCType(unsigned char entry_id);
	static uint64_t HandleLanguageUnit(QDomDocument& document, QDomNode& parent, const IFOFile& _ifo, int title);

	uint8_t familyUID[16];
	unsigned INDENT_COUNT;
};

#endif //CHAPTER_MANAGER_H
