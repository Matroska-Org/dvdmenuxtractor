#include "chaptermanager.h"
#include "utilities.h"

#include <QtXml>

QString ChapterManager::INFO_SUFFIX ("_info.xml");
QString ChapterManager::CHAPTER_SUFFIX ("_menu.xml");

bool ChapterManager::generateScript(const IFOFile& ifoFile, const QString& filename, uint16_t title, const QString& editionUID) const
{
	static unsigned char _PrivateVTS[] = {0x30, 0x80, 0x00, 0x00};
	static unsigned char _PrivateTT[]  = {0x28, 0x00, 0x00, 0x00};
	uint64_t start_timeH = uint64_t(-1);

	generateInfoScript(filename, title, editionUID);

	// create document
	QDomDocument document;

	// specify processing info
	document.appendChild(document.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\""));
	document.appendChild(document.implementation().createDocumentType("Chapters", QString::null, "matroskachapters.dtd"));

	// create comments
	document.appendChild(document.createComment("Created with " + Utilities::APPLICATION_NAME + " " + Utilities::APPLICATION_VERSION));

	// create the root element
	QDomNode root = document.createElement("Chapters");
	document.appendChild(root);

	// VTS content
	QDomElement editionEntryElement = document.createElement("EditionEntry");
	root.appendChild(editionEntryElement);
	
	editionEntryElement.appendChild(CreateDOMElement(document, "EditionUID", editionUID));
	editionEntryElement.appendChild(CreateDOMElement(document, "EditionFlagOrdered", "1"));
	editionEntryElement.appendChild(document.createComment("Video Title Set"));

	QDomElement chapterAtomElement = document.createElement("ChapterAtom");
	editionEntryElement.appendChild(chapterAtomElement);

	QString _myUIDa = Utilities::CreateUID();
	chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUIDa));
	chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterFlagHidden", "1"));
	
	_PrivateVTS[2] = title >> 8;
	_PrivateVTS[3] = title & 0xFF;
	AddCodecPrivateData(document, chapterAtomElement, _PrivateVTS, 4);

	QDomNode chapterAtomParentElement = chapterAtomElement;

	if (ifoFile.VtsTitles(title) && ifoFile.VtsPGCs(title))
	{
		for (int i = 0; i < ifoFile.VtsTitles(title)->nr_of_srpts; i++)
		{
			uint64_t start_time = uint64_t(-1), found_time;
			
			chapterAtomParentElement.appendChild(document.createComment(QString("Title TTU_%1").arg(i + 1)));
			QDomElement chapterAtomElement = document.createElement("ChapterAtom");
			chapterAtomParentElement.appendChild(chapterAtomElement);
			
			QString _myUIDa = Utilities::CreateUID();
			chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUIDa));
			chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterFlagHidden", "1"));

			// get the Title -> VTS+TTN map
			const tt_srpt_t *p_map = ifoFile.TitleMap();

			if ( p_map != NULL )
			{
				for (int j=0; j<p_map->nr_of_srpts; j++)
				{
					if ( ( p_map->title[j].title_set_nr == title ) && ( p_map->title[j].vts_ttn == i+1 ) )
					{
						_PrivateTT[1] = (j+1) >> 8;   // Title#
						_PrivateTT[2] = (j+1) & 0xFF;
						_PrivateTT[3] = i+1;          // VTS_TTN#
						AddCodecPrivateData(document, chapterAtomElement, _PrivateTT, 4);
						break;
					}
				}
			}

			// add the PGC that match this PTT
			for (int k = 0; k < ifoFile.VtsPGCs(title)->nr_of_pgci_srp; k++)
			{
				if ((ifoFile.VtsPGCs(title)->pgci_srp[k].entry_id & 0x7F) == i+1)
				{
					found_time = AddPGC(document, chapterAtomElement, 
						ifoFile.VtsPGCs(title)->pgci_srp[k].pgc, k, 0, ifoFile.CellsList(title, false), &ifoFile.VtsTitles(title)->title[i], i+1);
					
					if (start_time > found_time)
						start_time = found_time;
				}
			}

			chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_time)));

			if (start_timeH > start_time)
				start_timeH = start_time;
		}
	}

	if (start_timeH == uint64_t(-1))
		start_timeH = 0;

	chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_timeH)));
	
	QString output = document.toString(INDENT_COUNT);

	// TODO: not a good approach
	output.replace("-->", "-->\n");

	// chapter file
	QFile chapterFile (filename + CHAPTER_SUFFIX);
	if (!chapterFile.open(QIODevice::WriteOnly | QIODevice::Text))
		throw;

	chapterFile.write(output.toUtf8());
	chapterFile.close();

	return true;
}

bool ChapterManager::generateMenuScript(const IFOFile& ifoFile, const QString& filename, uint16_t title, const QString& editionUID) const
{
	static unsigned char _PrivateFP[]  = {0x30, 0x00, 0x00, 0x00};
	static unsigned char _PrivateVM[]  = {0x30, 0xC0, 0x00, 0x00};
	static unsigned char _PrivateVTS[] = {0x30, 0x40, 0x00, 0x00};

	// generate info script
	generateInfoScript(filename, title, editionUID);

	// create document
	QDomDocument document;

	// specify processing info
	document.appendChild(document.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\""));
	document.appendChild(document.implementation().createDocumentType("Chapters", QString::null, "matroskachapters.dtd"));

	// create comments
	document.appendChild(document.createComment("Created with " + Utilities::APPLICATION_NAME + " " + Utilities::APPLICATION_VERSION));

	// create the root element
	QDomNode parentElement = document.createElement("Chapters");
	document.appendChild(parentElement);

	// create Edition Entry Element and add to root
	QDomElement currentElement = document.createElement("EditionEntry");
	parentElement.appendChild(currentElement);
	
	// use Edition Entry Element as parent
	parentElement = currentElement;

	// add EditionUID and EDitionFlagOrdered elements
	parentElement.appendChild(CreateDOMElement(document, "EditionUID", editionUID));
	parentElement.appendChild(CreateDOMElement(document, "EditionFlagOrdered", "1"));

	// Write the first play PGC
	if (title == 0 && ifoFile.FirstPlayPGC())
	{
		// add comment to Edition Entry Element
		parentElement.appendChild(document.createComment("First Play PGC"));

		// add Chapter Atom
		QDomElement chapterAtomElement = document.createElement("ChapterAtom");
		parentElement.appendChild(chapterAtomElement);

		// create and set UID
		QString _myUID = Utilities::CreateUID();
		chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUID));

		// the First Play PGC has no cell and no time
		AddChapterTime(document, chapterAtomElement, 0, 0);

		// display the UID for the lazy rippers
		chapterAtomElement.appendChild(document.createComment("Enter your text here"));

		// add Chapter Display Element
		currentElement = document.createElement("ChapterDisplay");
		currentElement.appendChild(CreateDOMElement(document, "ChapterString", "First Play PGC " + _myUID));
		chapterAtomElement.appendChild(currentElement);

		// add Chapter Process Element
		currentElement = document.createElement("ChapterProcess");
		AddCodecPrivateData(document, currentElement, _PrivateFP, 4, false);
		chapterAtomElement.appendChild(currentElement);

		// PGC commands
		AddPGCCommands(document, currentElement, ifoFile.FirstPlayPGC()->command_tbl);
	}

	// Write the Video Title Set
	if (ifoFile.LanguageUnits(title) && ifoFile.LanguageUnits(title)->nr_of_lus)
	{
		// VTS Menu
		if ( title == 0 )
			parentElement.appendChild(document.createComment("Video Manager"));
		else
			parentElement.appendChild(document.createComment("Video Title Set"));

		// add another Chapter Atom element
		currentElement = document.createElement("ChapterAtom");
		parentElement.appendChild(currentElement);

		QString _myUIDa = Utilities::CreateUID();
		currentElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUIDa));
		currentElement.appendChild(CreateDOMElement(document, "ChapterFlagHidden", "1"));

		if ( title != 0 )
		{
			_PrivateVTS[2] = title >> 8;
			_PrivateVTS[3] = title & 0xFF;
			AddCodecPrivateData(document, currentElement, _PrivateVTS, 4);
		}
		else
		{
			_PrivateVM[2] = 0;
			_PrivateVM[3] = 0;
			AddCodecPrivateData(document, currentElement, _PrivateVM, 4);
		}

		uint64_t start_time = HandleLanguageUnit(document, currentElement, ifoFile, title);

		currentElement.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_time)));
	}

	// get document string representation
	QString output = document.toString(INDENT_COUNT);

	// TODO: not a good approach
	output.replace("-->", "-->\n");

	// chapter file
	QFile chapterFile(filename + CHAPTER_SUFFIX);
	if (!chapterFile.open(QIODevice::WriteOnly | QIODevice::Text))
		throw;
	
	// write to the chpater file
	chapterFile.write(output.toUtf8());
	chapterFile.close();

	return true;
}

void ChapterManager::generateInfoScript(const QString &filename, uint16_t title, const QString &editionUID) const
{
	// create document
	QDomDocument document;

	// specify xml version, encoding and DocType
	document.appendChild(document.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\""));
	document.appendChild(document.implementation().createDocumentType("Chapters", QString::null, "matroskainfos.dtd"));

	// add root element
	QDomNode parentElement = document.createElement("Info");
	document.appendChild(parentElement);

	// add Segment Family Element
	QDomElement currentElement = CreateDOMElement(document, "SegmentFamily", Utilities::EncodeHex(familyUID, 16));
	currentElement.setAttribute("format", "hex");
	parentElement.appendChild(currentElement);

	// add Chapter Translate Element
	currentElement = document.createElement("ChapterTranslate");
	currentElement.appendChild(CreateDOMElement(document, "ChapterTranslateEditionUID", editionUID));
	currentElement.appendChild(CreateDOMElement(document, "ChapterTranslateCodec", "1"));
	
	QDomElement chapterTranslateIDElement = CreateDOMElement(document, "ChapterTranslateID", QString("%1 00").arg(title, 2, 16, QChar('0')));
	chapterTranslateIDElement.setAttribute("format", "hex");
	currentElement.appendChild(chapterTranslateIDElement);

	parentElement.appendChild(currentElement);

	// get document string representation
	QString output = document.toString(INDENT_COUNT);
	
	// segment file
	QFile segementFile(filename + INFO_SUFFIX);
	if (!segementFile.open(QIODevice::WriteOnly | QIODevice::Text))
		throw;
	
	segementFile.write(output.toUtf8());
	segementFile.close();
}

QDomElement ChapterManager::CreateDOMElement(QDomDocument &document, const QString &elementName, const QString &content)
{
	QDomElement element = document.createElement(elementName);
	element.appendChild(document.createTextNode(content));

	return element;
}

void ChapterManager::AddChapterTime(QDomDocument& document, QDomNode& parent, uint64_t start_time, uint64_t end_time)
{
	parent.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_time)));
	parent.appendChild(CreateDOMElement(document, "ChapterTimeEnd", Utilities::FormatTime(end_time)));
}

void ChapterManager::AddCodecPrivateData(QDomDocument& document, QDomNode& parent, const unsigned char *buffer, unsigned int size, bool createChapterProcessElement /*= true*/)
{
	QDomNode node = parent;

	if (createChapterProcessElement)
	{
		QDomElement chapterProcessElement = document.createElement("ChapterProcess");
		parent.appendChild(chapterProcessElement);
		node = chapterProcessElement;
	}

	node.appendChild(CreateDOMElement(document, "ChapterProcessCodecID", "1"));

	QDomElement chapterProcessPrivateElement = CreateDOMElement(document, "ChapterProcessPrivate", Utilities::EncodeHex(buffer, size));
	chapterProcessPrivateElement.setAttribute("format", "hex");
	node.appendChild(chapterProcessPrivateElement);
}

void ChapterManager::AddPGCCommands(QDomDocument& document, QDomNode& parent, const pgc_command_tbl_t * command_tbl)
{
	if (command_tbl)
	{
		unsigned char *_buf = (unsigned char*) malloc(1 + command_tbl->nr_of_pre * 8);

		// Pre-commands
		if (command_tbl->nr_of_pre)
		{
			parent.appendChild(document.createComment("Pre commands"));

			QDomElement chapterCommandElement = document.createElement("ChapterProcessCommand");
			parent.appendChild(chapterCommandElement);

			QDomElement chapterProcessTimeElement = document.createElement("ChapterProcessTime");
			chapterProcessTimeElement.appendChild(document.createTextNode("1"));
			chapterCommandElement.appendChild(chapterProcessTimeElement);

			_buf[0] = command_tbl->nr_of_pre;
			memcpy(&_buf[1], command_tbl->pre_cmds, command_tbl->nr_of_pre * 8);

			QDomElement chapterProcessDataElement = CreateDOMElement(document, "ChapterProcessData", Utilities::EncodeHex(_buf, 1 + command_tbl->nr_of_pre * 8));
			chapterProcessDataElement.setAttribute("format", "hex");
			chapterCommandElement.appendChild(chapterProcessDataElement);
		}

		// Cell commands
		if (command_tbl->nr_of_cell)
		{
			parent.appendChild(document.createComment("Cell commands"));

			QDomElement chapterCommandElement = document.createElement("ChapterProcessCommand");
			parent.appendChild(chapterCommandElement);

			QDomElement chapterProcessTimeElement = document.createElement("ChapterProcessTime");
			chapterProcessTimeElement.appendChild(document.createTextNode("0"));
			chapterCommandElement.appendChild(chapterProcessTimeElement);

			_buf = (unsigned char*) realloc(_buf, 1 + command_tbl->nr_of_cell * 8);
			_buf[0] = command_tbl->nr_of_cell;
			memcpy(&_buf[1], command_tbl->cell_cmds, command_tbl->nr_of_cell * 8);

			QDomElement chapterProcessDataElement = CreateDOMElement(document, "ChapterProcessData", Utilities::EncodeHex(_buf, 1 + command_tbl->nr_of_cell * 8));
			chapterProcessDataElement.setAttribute("format", "hex");
			chapterCommandElement.appendChild(chapterProcessDataElement);
		}

		// Post commands
		if (command_tbl->nr_of_post)
		{
			parent.appendChild(document.createComment("Post commands"));

			QDomElement chapterCommandElement = document.createElement("ChapterProcessCommand");
			parent.appendChild(chapterCommandElement);

			QDomElement chapterProcessTimeElement = document.createElement("ChapterProcessTime");
			chapterProcessTimeElement.appendChild(document.createTextNode("2"));
			chapterCommandElement.appendChild(chapterProcessTimeElement);

			_buf = (unsigned char*) realloc(_buf, 1+command_tbl->nr_of_post * 8);
			_buf[0] = command_tbl->nr_of_post;
			memcpy(&_buf[1], command_tbl->post_cmds, command_tbl->nr_of_post * 8);

			QDomElement chapterProcessDataElement = CreateDOMElement(document, "ChapterProcessData", Utilities::EncodeHex(_buf, 1 + command_tbl->nr_of_post * 8));
			chapterProcessDataElement.setAttribute("format", "hex");
			chapterCommandElement.appendChild(chapterProcessDataElement);
		}

		free(_buf);
	}
}

uint64_t ChapterManager::HandleLanguageUnit(QDomDocument& document, QDomNode& parent, const IFOFile& _ifo, int title)
{
	static unsigned char _PrivateLU[]  = {0x2A, 0x00, 0x00, 0x00};
	uint64_t start_time = uint64_t(-1), found_time;

	QDomElement chapterAtomElement;
	QDomElement chapterDisplayElement;
	QDomElement chapterStringElement;
	QDomElement chapterTimeStartElement;

	for (int i = 0 ;i < _ifo.LanguageUnits(title)->nr_of_lus; i++)
	{
		parent.appendChild(document.createComment("Language Units"));

		chapterAtomElement = document.createElement("ChapterAtom");
		parent.appendChild(chapterAtomElement);

		// the Language Unit for a given language
		const pgci_lu_t &_LU = _ifo.LanguageUnits(title)->lu[i];
		_PrivateLU[1] = _LU.lang_code >> 8;
		_PrivateLU[2] = _LU.lang_code & 0xFF;
		_PrivateLU[3] = _LU.lang_extension;

		chapterAtomElement.appendChild(document.createComment(QString("Language: %1%2").arg(QChar(_LU.lang_code >> 8)).arg(QChar(_LU.lang_code & 0xFF))));
		chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", Utilities::CreateUID()));

		AddCodecPrivateData(document, chapterAtomElement, _PrivateLU, 4);

		// display the UID for the lazy rippers
		chapterAtomElement.appendChild(document.createComment("Enter your text here"));

		chapterDisplayElement = document.createElement("ChapterDisplay");
		chapterAtomElement.appendChild(chapterDisplayElement);

		chapterStringElement = document.createElement("ChapterString");
		chapterStringElement.appendChild(document.createTextNode(QString("Language Unit for %1%2").arg(QChar(_LU.lang_code >> 8)).arg(QChar(_LU.lang_code & 0xFF))));
		chapterDisplayElement.appendChild(chapterStringElement);

		if (_LU.pgcit)
		{
			for (int j = 0; j < _LU.pgcit->nr_of_pgci_srp; j++)
			{
				const pgci_srp_t &_PGC_SRP = _LU.pgcit->pgci_srp[j];

				found_time = AddPGC(document, chapterAtomElement, _PGC_SRP.pgc, j, _PGC_SRP.entry_id, _ifo.CellsList(title, true), NULL, 0);

				if (start_time > found_time)
					start_time = found_time;
			}
		}

		chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_time)));
	}

	return start_time;
}


uint64_t ChapterManager::AddPGC(QDomDocument& document, QDomNode& parent, const pgc_t * pgc, uint16_t pgc_num, unsigned char pgc_type, const CellsListType & cell_list, const ttu_t * ptts, int title)
{
	static unsigned char _PrivatePGC[] = {0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint64_t start_time = uint64_t(-1), found_time;

	parent.appendChild(document.createComment(QString("PGC PGC_%1 type %2").arg(pgc_num+1).arg(GetPGCType(pgc_type))));

	QDomElement chapterAtomElement = document.createElement("ChapterAtom");
	parent.appendChild(chapterAtomElement);

	QString _myUID = Utilities::CreateUID();
	chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUID));

	// display the UID for the lazy rippers
	if (ptts == NULL)
	{
		chapterAtomElement.appendChild(document.createComment("Enter your text here"));

		QDomElement chapterDisplayElement = document.createElement("ChapterDisplay");
		chapterDisplayElement.appendChild(CreateDOMElement(document, "ChapterString", "PGC type " + GetPGCType(pgc_type)));
		chapterAtomElement.appendChild(chapterDisplayElement);
	}
	else
		chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterFlagHidden", "1"));

	// PGC commands
	QDomElement chapterProcessElement = document.createElement("ChapterProcess");
	chapterAtomElement.appendChild(chapterProcessElement);

	_PrivatePGC[1] = (pgc_num+1) >> 8;
	_PrivatePGC[2] = (pgc_num+1) & 0xFF;
	_PrivatePGC[3] = pgc_type;
	_PrivatePGC[4] = ((uint8_t*) &pgc->prohibited_ops)[0];
	_PrivatePGC[5] = ((uint8_t*) &pgc->prohibited_ops)[1];
	_PrivatePGC[6] = ((uint8_t*) &pgc->prohibited_ops)[2];
	_PrivatePGC[7] = ((uint8_t*) &pgc->prohibited_ops)[3];

	AddCodecPrivateData(document, chapterProcessElement, _PrivatePGC, 8, false);
	AddPGCCommands(document, chapterProcessElement, pgc->command_tbl);

	// Handle the Programs/Cells in the PGC
	for (int i=0; i< pgc->nr_of_programs; i++)
	{
		found_time = AddProgram(document, chapterAtomElement, pgc, _myUID, i, cell_list, pgc_num, ptts, title);

		if (start_time > found_time)
			start_time = found_time;
	}

	if (start_time == uint64_t(-1))
		start_time = 0;

	chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_time)));

	return start_time;
}


QString ChapterManager::GetPGCType(unsigned char entry_id)
{
	QString result ("0x%1 = ");
	result = result.arg(entry_id, 2, 16, QChar('0'));

	if (entry_id & 0x80)
		result += "Entry + ";

	switch (entry_id & ~0x80)
	{
	case 2:
		result += "Title-Menu";
		break;
	case 3:
		result += "Root Menu";
		break;
	case 4:
		result += "Subpicture Menu";
		break;
	case 5:
		result += "Audio Menu";
		break;
	case 6:
		result += "Angle Menu";
		break;
	case 7:
		result += "Chapter Menu";
		break;
	default:
		result += "Basic PGC";
		break;
	}

	return result;
}


uint64_t ChapterManager::AddProgram(QDomDocument& document, QDomNode& parent, const pgc_t *pgc, const QString& PgcUID, int program_number, const CellsListType& cell_list, int16_t pgc_num, const ttu_t *ptts, int title)
{
	static unsigned char _PrivatePTT[] = {0x10, 0x00};
	static unsigned char _PrivatePG[] = {0x18, 0x00, 0x00};
	static unsigned char _PrivateCN[] = {0x08, 0x00, 0x00, 0x00, 0x00};

	uint64_t start_time = uint64_t(-1), end_time = 0;

	// for this program
	int entry_cell_num = pgc->program_map[program_number];
	int program_last_cell_num = pgc->nr_of_cells;
	int next_program_entry_cell_num;
	if (program_number+1 == pgc->nr_of_programs)
		next_program_entry_cell_num = program_last_cell_num+1;
	else
		next_program_entry_cell_num = pgc->program_map[program_number+1];

	parent.appendChild(document.createComment(QString("Program PGN# %1 in this PGC").arg(program_number + 1)));

	QDomElement chapterAtomElement = document.createElement("ChapterAtom");
	parent.appendChild(chapterAtomElement);

	QString _myUID = Utilities::CreateUID();
	chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUID));
	chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterFlagHidden", "1"));

	_PrivatePG[1] = (program_number+1) >> 8;
	_PrivatePG[2] = (program_number+1) & 0xFF;
	AddCodecPrivateData(document, chapterAtomElement, _PrivatePG, 3);

	if (ptts != NULL)
	{
		// find all PTT corresponding to this PGC/PG number
		for (uint16_t pppt_nr = 0; pppt_nr < ptts->nr_of_ptts; pppt_nr++)
		{
			// use the start timecode of the first cell
			const cell_position_t & position = pgc->cell_position[entry_cell_num-1];
			const CellListElem * cell_elt = cell_list.at(position);
			// TODO: make this test optional (create chapters even without the content)
			if (cell_elt == NULL || !cell_elt->found)
				continue;
			
			start_time = cell_elt->start_time;

			QDomElement chapterAtomParent = chapterAtomElement;

			if (ptts->ptt[pppt_nr].pgcn == pgc_num+1 && ptts->ptt[pppt_nr].pgn == program_number+1)
			{
				chapterAtomElement.appendChild(document.createComment(QString("Chapter PTT#%1 [%2.%3]").arg(pppt_nr+1).arg(pgc_num+1, 2, 10, QChar('0')).arg(program_number+1, 2, 10, QChar('0'))));
				
				chapterAtomElement = document.createElement("ChapterAtom");
				chapterAtomParent.appendChild(chapterAtomElement);

				chapterAtomElement.appendChild(document.createComment("Enter your text here"));

				QDomElement chapterDisplayElement = document.createElement("ChapterDisplay");
				chapterDisplayElement.appendChild(CreateDOMElement(document, "ChapterString", QString("Your Name Here For Chapter #%1.%2").arg(title).arg(pppt_nr + 1)));
				chapterAtomElement.appendChild(chapterDisplayElement);

				QString _myUID = Utilities::CreateUID();
				chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUID));

				_PrivatePTT[1] = pppt_nr+1;
				AddCodecPrivateData(document, chapterAtomElement, _PrivatePTT, 2);

				chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_time)));
			}

			chapterAtomElement = chapterAtomParent;
		}
	}

	QDomElement parentChapterAtom = chapterAtomElement;

	for (int cell_num = entry_cell_num; cell_num<next_program_entry_cell_num; cell_num++)
	{
		// Output cells
		const cell_position_t & position = pgc->cell_position[cell_num-1];
		const cell_playback_t & cell = pgc->cell_playback[cell_num-1];

		if (cell.block_mode != 0 && cell.block_mode != 3)
			continue;

		const CellListElem * cell_elt = cell_list.at(position);
		// TODO: make this test optional (create chapters even without the content)
		if (cell_elt == NULL || !cell_elt->found)
			continue;

		QString prefix = QString("Program %1 Cell CN#%2 ").arg( (cell_num == entry_cell_num) ? "Entry" : "", QString::number(cell_num));
		QString middle = QString("[%1.%2]").arg(cell_elt->vobid, 2, 10, QChar('0')).arg(cell_elt->cellid);
		QString suffix = QString(" (%1 angles)").arg(cell_num - entry_cell_num + 1);

		if (cell.block_mode) // multi-angle
			parentChapterAtom.appendChild(document.createComment(prefix + middle + suffix));
		else
			parentChapterAtom.appendChild(document.createComment(prefix + middle));

		QDomElement chapterAtomElement = document.createElement("ChapterAtom");
		parentChapterAtom.appendChild(chapterAtomElement);

		QString _myUID = Utilities::CreateUID();
		chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterUID", _myUID));
		chapterAtomElement.appendChild(CreateDOMElement(document, "ChapterFlagHidden", "1"));

		_PrivateCN[1] = (position.vob_id_nr) >> 8;
		_PrivateCN[2] = (position.vob_id_nr) & 0xFF;
		_PrivateCN[3] = position.cell_nr;
		_PrivateCN[4] = cell_num - entry_cell_num + 1; // Number of angles
		AddCodecPrivateData(document, chapterAtomElement, _PrivateCN, 5);

		if (cell.still_time == 0xFF) {
			chapterAtomElement.appendChild(document.createComment("Infinite loop Still Cell"));

			// output an additional tag to specify this is cell should loop infinitely
			// post-process command, jump to timecode "st_time"
			QDomElement chapterProcessElement = document.createElement("ChapterProcess");
			chapterProcessElement.appendChild(CreateDOMElement(document, "ChapterProcessCodecID", "0"));
			chapterProcessElement.appendChild(document.createComment("Post command: replay this cell"));
			chapterAtomElement.appendChild(chapterProcessElement);

			QDomElement chapterProcessCommand = document.createElement("ChapterProcessCommand");
			chapterProcessCommand.appendChild(CreateDOMElement(document, "ChapterProcessTime", "2"));
			chapterProcessElement.appendChild(chapterProcessCommand);

			QDomElement chapterProcessDataElement = CreateDOMElement(document, "ChapterProcessData", "GotoAndPlay(" + PgcUID + ");" );
			chapterProcessDataElement.setAttribute("format", "ascii");
			chapterProcessCommand.appendChild(chapterProcessDataElement);
		} else if (cell.still_time != 0)
			chapterAtomElement.appendChild(document.createComment(QString("Still cell (%1s)").arg(cell.still_time)));

		AddChapterTime(document, chapterAtomElement, cell_elt->start_time, cell_elt->start_time + cell_elt->duration);

		if (end_time < cell_elt->start_time + cell_elt->duration)
			end_time = cell_elt->start_time + cell_elt->duration;

		if (start_time > cell_elt->start_time)
			start_time = cell_elt->start_time;
	}

	if (start_time == uint64_t(-1))
		start_time = 0; // unknown

	parentChapterAtom.appendChild(CreateDOMElement(document, "ChapterTimeStart", Utilities::FormatTime(start_time)));

	return start_time;
}
