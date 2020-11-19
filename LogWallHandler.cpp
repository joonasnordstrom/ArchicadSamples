#include "stdafx.h"

#include <AngleData.h>

#include <list>
#include <vector>
#include <algorithm>

//#ifdef _WINDOWS
//#include <shlobj.h>
//#endif



#include "../../../clib/polywork.h"
#include "../../../clib/acadutil.h"
#include "../../../cpplib/ph_acadutil.h"
#include "../../../cpplib/dgmisc.h"

#include "logwallhandler.h"

#include	"ProfileVectorImage.hpp"
#include	"VectorImageIterator.hpp"
#include	"ProfileAdditionalInfo.hpp"
#include	"ProfileVectorImageOperations.hpp"


#include "owndata.h"
#include "logobject.h"
#include "resid.h"
#include "objobserver.h"
#include "toolpalette.h"

// Own data saved to attribute
#define ATTR_PROFILE_ID "APID"
#define ATTR_BOTTOMS_ID "ABID"

static const std::string grProfilePrefix = "LOG_";			// Prefix of attribute name to be considered as log wall

CCreateLogsDlg	*_prLogWallPalette = nullptr;

template <class T>
static bool VectorContainsValue(const std::vector<T> &vec, const T &val)
{
	if (std::find(vec.begin(), vec.end(), val) == vec.end())
	{
		return false;
	}

	return true;
}

CCreateLogsDlg::CCreateLogsDlg() :
	DG::Palette(PH_RES_CONSTRUCTOR(short) (grOptions.m_nResIdBase + IDD_OSTLAFTLOGS) PH_RES_CONSTRUCTOR_POST),
	m_butApply(GetReference(), 1),
	m_totalHeight(GetReference(), 2),
	m_logList(GetReference(), 3),
	m_logTypes(GetReference(), 6),
	m_logWidths(GetReference(), 7),
	m_profileList(GetReference(), 9),
	m_txtProfileName(GetReference(), 11),
	m_butSaveProfile(GetReference(), 14),
	m_butDiscardChanges(GetReference(), 15),
	m_butSaveName(GetReference(), 16),
	m_butDiscardName(GetReference(), 17),
	m_butAddNewProfile(GetReference(), 18),
	m_butDuplicateProfile(GetReference(), 19),
	m_butDeleteProfile(GetReference(), 20),
	m_butAddNewLog(GetReference(), 21),
	m_butDeleteLog(GetReference(), 22),
	m_butMoveUp(GetReference(), 23),
	m_butMoveDown(GetReference(), 24),
	m_logBottoms(GetReference(), 26)
{
	Attach(*this);

	m_butApply.Attach(*this);
	m_butAddNewLog.Attach(*this);
	m_butDeleteLog.Attach(*this);
	m_butMoveUp.Attach(*this);
	m_butMoveDown.Attach(*this);
	m_logTypes.Attach(*this);
	m_logList.Attach(*this);
	m_logWidths.Attach(*this);
	m_profileList.Attach(*this);
	m_butDuplicateProfile.Attach(*this);
	m_butDeleteProfile.Attach(*this);
	m_butAddNewProfile.Attach(*this);
	m_butSaveProfile.Attach(*this);
	m_butDiscardChanges.Attach(*this);
	m_txtProfileName.Attach(*this);
	m_butSaveName.Attach(*this);
	m_butDiscardName.Attach(*this);
	m_logBottoms.Attach(*this);

	m_dTotalHeight = 0;

	if (m_rLogTypeHandler.IsInited())
	{
		PopulateLogWidths();
		PopulateLogTypes();
		PopulateLogBottoms();
		DrawProfileList();
	}
	else
	{
		ph::AcadErrMsg("Log profiles were not found. Make sure you have imported all necessary libraries.");
	}

	grElemNotify.AddObserver(this);

	m_bIsSavedState = true;
}


CCreateLogsDlg::~CCreateLogsDlg()
{
	Detach(*this);

	m_butApply.Detach(*this);
	m_butAddNewLog.Detach(*this);
	m_butDeleteLog.Detach(*this);
	m_butMoveUp.Detach(*this);
	m_butMoveDown.Detach(*this);
	m_logTypes.Detach(*this);
	m_logList.Detach(*this);
	m_logWidths.Detach(*this);
	m_profileList.Detach(*this);
	m_butDuplicateProfile.Detach(*this);
	m_butDeleteProfile.Detach(*this);
	m_butAddNewProfile.Detach(*this);
	m_butSaveProfile.Detach(*this);
	m_butDiscardChanges.Detach(*this);
	m_txtProfileName.Detach(*this);
	m_butSaveName.Detach(*this);
	m_butDiscardName.Detach(*this);
	m_logBottoms.Detach(*this);

	_prLogWallPalette = nullptr;

	grElemNotify.DelObserver(this);
}



void CCreateLogsDlg::OnDocChanged()
{
	// repopulate dialog if project changes
	if (_prLogWallPalette)
	{
		LogWall_Destroy();
		LogWall_Toggle();
	}
}


void CCreateLogsDlg::SetUIStatus()
{
}

void CCreateLogsDlg::Show()
{
	BeginEventProcessing();

	DG::Palette::Show();
}



/*
*
* EVENTS
*
*/
void CCreateLogsDlg::PanelOpened(const DG::PanelOpenEvent& /*ev*/)
{
	short nSelectedProfile = m_profileList.GetSelectedItem();

	if (nSelectedProfile > 0)
	{
		GetSelectedProfile(m_rProfileName);

		m_txtProfileName.SetText(m_rProfileName);
	}
	// if no profile is selected, lets populate profile name with prefix
	else
	{
		m_txtProfileName.SetText(grProfilePrefix.c_str());
	}
}

void CCreateLogsDlg::PanelClosed(const DG::PanelCloseEvent &ev)
{
	if (_prLogWallPalette == this) _prLogWallPalette = nullptr;
}

void CCreateLogsDlg::PanelCloseRequested(const DG::PanelCloseRequestEvent& ev, bool* accept)
{
	EndEventProcessing();
	return;
}

void CCreateLogsDlg::OnElemSelChange(API_Element *prElem, const API_Neig *selElemNeig)
{
	if (prElem)
	{
		if (prElem->header.typeID == API_WallID)
		{
			API_Attribute rAttr;

			GSErrCode nErr = CAttributeHelpers::GetAttributeByIndx(prElem->wall.profileAttr, API_ProfileID, rAttr);

			if (nErr == NoError)
			{
				if (!m_bIsSavedState)
				{
					QueryForPendingChanges();
				}

				PopulateUIByAttribute(rAttr);

				m_bIsSavedState = true;
			}
		}
	}
}


void CCreateLogsDlg::CheckItemChanged(const DG::CheckItemChangeEvent& /*ev*/) {}

void CCreateLogsDlg::TextEditChanged(const DG::TextEditChangeEvent& ev)
{
	if (ev.GetSource() == &m_txtProfileName)
	{
		return;
	}
}

void CCreateLogsDlg::ListBoxSelectionChanged(const DG::ListBoxSelectionEvent& ev)
{
	try
	{
		if (ev.GetSource() == &m_logList)
		{
			HandleLogListEvent();
		}
		else if (ev.GetSource() == &m_profileList)
		{
			HandleProfileListEvent();
		}
	}
	catch (std::exception &e)
	{
		grFrameSettings.HandleException(e);
	}
}

void CCreateLogsDlg::UserControlChanged(const DG::UserControlChangeEvent& ev)
{
	try
	{
		if (ev.GetSource() == &m_logTypes)
		{
			HandleLogTypesEvent();
		}
		else if (ev.GetSource() == &m_logWidths)
		{
			HandleLogWidthsEvent();
		}
		else if (ev.GetSource() == &m_logBottoms)
		{
			HandleLogBottomsEvent();
		}
	}
	catch (std::exception &e)
	{
		grFrameSettings.HandleException(e);
	}
}

void CCreateLogsDlg::ButtonClicked(const DG::ButtonClickEvent& ev)
{
	try
	{
		if (ev.GetSource() == &m_butApply)
		{
			HandleApplyEvent();
		}
		else if (ev.GetSource() == &m_butAddNewLog)
		{
			HandleAddLogEvent();
		}
		else if (ev.GetSource() == &m_butDeleteLog)
		{
			HandleDeleteLogEvent();
		}
		else if (ev.GetSource() == &m_butMoveUp)
		{
			HandleMoveUpEvent();
		}
		else if (ev.GetSource() == &m_butMoveDown)
		{
			HandleMoveDownEvent();
		}
		else if (ev.GetSource() == &m_butSaveName)
		{
			HandleSaveNameEvent();
		}
		else if (ev.GetSource() == &m_butDiscardName)
		{
			HandleDiscardNameEvent();
		}
		else if (ev.GetSource() == &m_butAddNewProfile)
		{
			HandleAddNewProfileEvent();
		}
		else if (ev.GetSource() == &m_butDeleteProfile)
		{
			HandleDeleteProfileEvent();
		}
		else if (ev.GetSource() == &m_butDuplicateProfile)
		{
			HandleDuplicateProfileEvent();
		}
		else if (ev.GetSource() == &m_butDiscardChanges)
		{
			HandleDiscardChangesEvent();
		}
		else if (ev.GetSource() == &m_butSaveProfile)
		{
			HandleSaveProfileEvent();
		}
	}
	catch (std::exception &e)
	{
		grFrameSettings.HandleException(e);
	}
}


/*
*
* GUI item event handler functions
*
*/
void CCreateLogsDlg::HandleDiscardNameEvent()
{
	m_txtProfileName.SetText(m_rProfileName);
}

void CCreateLogsDlg::HandleSaveNameEvent()
{
	if (ValidateProfileName())
	{
		if (!DoesProfileNameAlreadyExist(m_txtProfileName.GetText().ToCStr()))
		{
			if (m_bIsSavedState == false)
			{
				assert(!m_rProfileName.IsEmpty());
				QueryForPendingChanges(m_rProfileName.ToCStr());
			}

			UpdateProfileName();
			m_bIsSavedState = true;
		}
		else
		{
			ph::AcadErrMsg("Profile by that name already exists.");
		}
	}
}

void CCreateLogsDlg::HandleLogListEvent()
{
	UpdateLogListUI();
}

void CCreateLogsDlg::HandleLogBottomsEvent()
{
	short nSelectedLog = m_logList.GetSelectedItem();

	if (nSelectedLog <= 0) return;

	CLogItem *prLog = m_rLogItemHandler.GetLogById(nSelectedLog);

	prLog->SetBottomType(m_logBottoms.GetValue());

	UpdateLogList(nSelectedLog);
	UpdateTotalHeight();

	m_bIsSavedState = false;
}


void CCreateLogsDlg::HandleLogWidthsEvent()
{
	GS::Int32 nSelectedWidth = m_logWidths.GetValue();

	if (nSelectedWidth <= 0) return;

	auto rSelectedWidth = m_rLogTypeHandler.GetWidthByID(nSelectedWidth);

	m_rLogTypeHandler.UpdateLogTypesByWidth(rSelectedWidth->dValue);

	PopulateLogTypes();

	short nSelectedLogID = m_logList.GetSelectedItem();

	if (nSelectedLogID > 0)
	{
		auto prLog = m_rLogItemHandler.GetLogById(nSelectedLogID);

		if (prLog)
		{
			auto prNewLogType = m_rLogTypeHandler.GetLogTypeById(m_logTypes.GetValue());

			if (prNewLogType)
			{
				prLog->SetType(prNewLogType);

				UpdateLogList(nSelectedLogID);

				m_bIsSavedState = false;
			}
		}
	}
}

void CCreateLogsDlg::HandleMoveUpEvent()
{
	short nSelectedLog = m_logList.GetSelectedItem();

	if (m_rLogItemHandler.MoveLog(true, m_logList.GetSelectedItem()))
	{
		UpdateLogList(nSelectedLog);
		m_bIsSavedState = false;
	}
}

void CCreateLogsDlg::HandleMoveDownEvent()
{
	short nSelectedLog = m_logList.GetSelectedItem();

	if (m_rLogItemHandler.MoveLog(false, m_logList.GetSelectedItem()))
	{
		UpdateLogList(nSelectedLog);
		m_bIsSavedState = false;
	}
}

void CCreateLogsDlg::HandleLogTypesEvent()
{
	short nSelectedLog = m_logList.GetSelectedItem();
	GS::Int32 nSelectedType = m_logTypes.GetValue();

	if (nSelectedLog <= 0 || nSelectedType <= 0) return;

	const CLogType *prNewType = m_rLogTypeHandler.GetLogTypeById(static_cast<size_t>(nSelectedType));

	CLogItem *prLog = m_rLogItemHandler.GetLogById(nSelectedLog);

	if (prLog && prNewType)
	{
		// update total height
		m_dTotalHeight += prNewType->GetHeight() - prLog->GetType()->GetHeight();

		prLog->SetType(prNewType);

		UpdateLogList(nSelectedLog);
		UpdateTotalHeight();

		m_bIsSavedState = false;
	}
}

void CCreateLogsDlg::HandleProfileListEvent()
{
	short nSelectedItem = m_profileList.GetSelectedItem();

	if (nSelectedItem == 0) return;

	GS::UniString rSelectedProfileName = m_profileList.GetTabItemText(nSelectedItem, 1);

	m_txtProfileName.SetText(rSelectedProfileName);

	if (!m_bIsSavedState)
	{
		// Update previous profile
		QueryForPendingChanges(m_rProfileName.ToCStr());
	}

	m_rProfileName = rSelectedProfileName;

	API_Attribute rSelectedAttr;

	GSErrCode nErr = CAttributeHelpers::GetAttributeByName(rSelectedProfileName.ToCStr(), API_ProfileID, rSelectedAttr);

	if (nErr != NoError) ph::CAcadException::Throw(nErr, "Error trying to get attribute.");

	PopulateUIByAttribute(rSelectedAttr);

	m_bIsSavedState = true;
}

void CCreateLogsDlg::HandleApplyEvent()
{
PH_ACADUTIL_UNDO_BEGIN("ArchiLogs set wall profile")
	API_Attribute	rSelectedAttr;
	GS::UniString	rAttrName;
	ph::CAcadSuspendGroups	suspend(true);

	GetSelectedProfile(rAttrName);

	if (!rAttrName.IsEmpty())
	{
		CAttributeHelpers::GetAttributeByName(rAttrName.ToCStr(), API_ProfileID, rSelectedAttr);
		m_rLogsHandler.ApplyLogProfileToSelectedWalls(rSelectedAttr);
	}
PH_ACADUTIL_UNDO_END_VOID()
}

void CCreateLogsDlg::HandleAddNewProfileEvent()
{
	if (m_bIsSavedState == false)
	{
		QueryForPendingChanges();
	}

	AddProfile();
	SetDefaults();
	Save();
	m_bIsSavedState = true;
}

void CCreateLogsDlg::HandleSaveProfileEvent()
{
	Save();

	m_bIsSavedState = true;
}

void CCreateLogsDlg::HandleDiscardChangesEvent()
{
	int nResponse = ph::AcadQueryMsgBox(MB_OKCANCEL, "Are you sure you want to revert all pending changes?");

	if (nResponse == 1)
	{
		RevertPendingChanges();
	}

	m_bIsSavedState = true;
}

void CCreateLogsDlg::HandleDuplicateProfileEvent()
{
	if (!m_bIsSavedState)
	{
		QueryForPendingChanges();
	}
	AddProfile();
	Save();
	m_bIsSavedState = true;
}

void CCreateLogsDlg::HandleDeleteProfileEvent()
{
	DeleteProfile();
	m_bIsSavedState = true;
}

void CCreateLogsDlg::HandleAddLogEvent()
{
	// new log position is below selected log, 0 = no select
	//short nSelectedLogIndx = m_logList.GetSelectedItem();

	if (m_profileList.GetSelectedItem() == 0)
	{
		ph::AcadMsgBox("Please select a profile you wish to edit");
		return;
	}

	GS::Int32 nLogTypeId = m_logTypes.GetValue();

	// new log always after selected one
	short nLogIndx = m_logList.GetSelectedItem() + 1;


	const CLogType *prType = m_rLogTypeHandler.GetLogTypeById(nLogTypeId);

	assert(prType);

	if (prType)
	{
		CLogItem	rLog(nLogIndx, m_logBottoms.GetValue(), prType);

		m_rLogItemHandler.AddLog(rLog);

		m_dTotalHeight += prType->GetHeight();

		UpdateLogList(static_cast<short>(m_rLogItemHandler.GetCount()));
		UpdateTotalHeight();

		m_bIsSavedState = false;
	}
}

void CCreateLogsDlg::HandleDeleteLogEvent()
{
	DeleteSelectedLog();

	m_bIsSavedState = false;
}


/*
*
* CCreateLogsDlg functions
*
*/
void CCreateLogsDlg::PopulateUIByAttribute(const API_Attribute &rAttr)
{
	CLogWallAttrData		attrData;
	//std::vector<std::string> vecLogNames;
	//std::vector<GS::Int32> vecBottomTypes;

	std::string rAttrName = rAttr.header.name;

	// lets make sure that it truly is our "LOG_" prefixed profile attribute
	if (rAttr.header.typeID != API_ProfileID || rAttrName.find(grProfilePrefix.c_str(), 0) == rAttrName.npos) return;

	//m_rProfileAttributeHandler.ReadProfileData(rAttr.header, vecLogNames, vecBottomTypes);
	attrData.FromAttr(rAttr.header);

	m_rLogItemHandler.DeleteLogs();
	ClearLogList();

	if (attrData.m_vecProfileTypes.size() != attrData.m_vecLogNames.size()) ph::CAcadException::Throw(Error, "Profile has mixed user data. Please delete/recreate profile.");

	// TODO figure out why sometimes logtypes not inited, it has something to do with OnDocChanged() event
	if (!m_rLogTypeHandler.IsInited())
	{
		m_rLogTypeHandler.Init();
	}

	for (size_t i = 0; i < attrData.m_vecLogNames.size(); i++)
	{
		auto prCurrLogType = m_rLogTypeHandler.GetLogTypeByName(attrData.m_vecLogNames.at(i));

		assert(prCurrLogType);
		if (!prCurrLogType) ph::CAcadException::Throw(Error, "Null pointer exception");

		CLogItem rCurrLog;

		rCurrLog.SetType(prCurrLogType);
		rCurrLog.SetBottomType(attrData.m_vecProfileTypes.at(i));

		m_rLogItemHandler.AddLog(rCurrLog);
	}

	short nProfileCount = m_profileList.GetItemCount();

	short i = 1;

	for (; i <= nProfileCount; i++)
	{
		GS::UniString rCurrProfileName = m_profileList.GetTabItemText(i, 1);

		if (rCurrProfileName == rAttr.header.name)
		{
			m_profileList.SelectItem(i);
			m_txtProfileName.SetText(rCurrProfileName);
			break;
		}
	}

	// if attribute was found in list, populate logs
	if (i <= nProfileCount)
	{
		DrawLogList();
	}

	UpdateTotalHeight();
}

void CCreateLogsDlg::RevertPendingChanges()
{
	auto nSelectedProfileID = m_profileList.GetSelectedItem();

	auto rSelectedProfileName = m_profileList.GetTabItemText(nSelectedProfileID, 1);

	API_Attribute rAttr;

	GSErrCode nErr = CAttributeHelpers::GetAttributeByName(rSelectedProfileName.ToCStr(), API_ProfileID, rAttr);

	if (nErr != NoError) ph::CAcadException::Throw(nErr, "Trying to get attribute failed.");

	PopulateUIByAttribute(rAttr);
}


bool CCreateLogsDlg::ValidateProfileName()
{
	auto rProfileName = m_txtProfileName.GetText();

	if (rProfileName.FindFirst(grProfilePrefix.c_str(), 0) == 0)
	{
		return true;
	}

	ph::AcadMsgBox("Profile name must begin with prefix: \"%s\"", grProfilePrefix.c_str());

	return false;
}

bool CCreateLogsDlg::DoesProfileNameAlreadyExist(const char *pProfileName)
{
	for (size_t i = 0; i < m_profileList.GetItemCount(); i++)
	{
		GS::UniString rCurrProfileName = m_profileList.GetTabItemText((short)i + 1, 1);

		if (rCurrProfileName == pProfileName) {
			return true;
		}
	}

	return false;
}

void CCreateLogsDlg::PopulateLogWidths()
{
	DGMiscUC257Filler	filler(0);
	int					firstSel = 1;
	char buf[128];

	for (size_t i = 0; i < m_rLogTypeHandler.GetWidthCount(); i++)
	{
		auto rCurrWidth = m_rLogTypeHandler.GetWidthByIndx(i);

		ph::AcadNumToString(rCurrWidth->dValue, DG_ET_LENGTH, buf, sizeof(buf), ph::EAcadNumPrefDimensions, NULL);

		filler.SetItem(static_cast<int>(rCurrWidth->nID - 1), buf);
	}

	filler.ApplyToControl(m_logWidths);
	DGMISC_SetUC257Value(m_logWidths, firstSel, filler.GetCount());
}

void CCreateLogsDlg::Save(const char *pProfileName)
{
	if (ValidateProfileName())
	{
		API_Attribute		rProfileAttr;
		API_AttributeDefExt	rProfileAttrDef;

		// good enough
		std::string rProfileName = pProfileName ? pProfileName : m_txtProfileName.GetText().ToCStr();

		try {
			m_rLogsHandler.BuildProfileAttribute(*m_rLogItemHandler.GetLogList(), rProfileName, rProfileAttr, rProfileAttrDef);
		}
		catch (std::exception &e) 
		{
			assert(false);
			ACAPI_DisposeAttrDefsHdlsExt(&rProfileAttrDef);
			throw e;
		}

		ph::CAcadChangeDB rTo2D;

		GSErrCode nErr = m_rLogsHandler.CreateProfile(rProfileAttr, rProfileAttrDef);

		if (nErr == APIERR_ATTREXIST)
		{
			nErr = m_rLogsHandler.UpdateProfile(rProfileAttr, rProfileAttrDef);
		}

		if (nErr != NoError) ph::CAcadException::Throw(nErr, "Error trying to create profile.");

		if ((nErr = m_rProfileAttributeHandler.SaveProfileData(rProfileAttr.header, m_rLogItemHandler)) != NoError) ph::CAcadException::Throw(nErr, "Error trying to save profile data.");
	}
}

void CCreateLogsDlg::UpdateProfileName()
{
	API_Attribute			rProfileAttr;

	GS::UniString rNewProfileName = m_txtProfileName.GetText();

	GSErrCode nErr = CAttributeHelpers::GetAttributeByName(m_rProfileName.ToCStr(), API_ProfileID, rProfileAttr);

	if (nErr == NoError)
	{
		if ((nErr = ACAPI_Attribute_Delete(&rProfileAttr.header)) != NoError) ph::CAcadException::Throw(nErr, "Error trying to delete attribute.");

		Save();
	}

	// APIERR_BADNAME = profile not created yet
	if (nErr == APIERR_BADNAME || nErr == NoError)
	{
		nErr = NoError;

		m_profileList.SetTabItemText(m_profileList.GetSelectedItem(), 1, rNewProfileName);

		m_rProfileName = rNewProfileName;
	}

	if (nErr != NoError) ph::CAcadException::Throw(nErr, "Error trying to modify profile.");
}

/* PH poisti
void CCreateLogsDlg::UpdateWallElemData()
{
	ph::CElemListHolder		rElems;
	rElems.GetAll(API_WallID, 0, true);
	//for (size_t i = 0; i < rElems.GetCount(); i++)
	//{
	//	rElems.GetElement()
	//}
}
*/

void CCreateLogsDlg::AddProfile()
{
	std::string rProfileName = (const char *)m_txtProfileName.GetText().ToCStr();
	//int nDuplicateCount = 1;

	if (rProfileName.empty())
	{
		ph::AcadMsgBox("Profile name must not be empty.");
		return;
	}

	short nProfileCount = m_profileList.GetItemCount();
	m_profileList.InsertItem(nProfileCount + 1);

	int nDuplicatesFound = GetProfileDuplicateCount();

	// Duplicates found
	if (nDuplicatesFound > 0)
	{
		rProfileName = rProfileName.substr(0, rProfileName.find('('));

		rProfileName = rProfileName + "(" + std::to_string(nDuplicatesFound) + ")";
	}

	m_profileList.SetTabItemText(nProfileCount + 1, 1, rProfileName.c_str());
	m_profileList.SelectItem(nProfileCount + 1);
	m_txtProfileName.SetText(rProfileName.c_str());
	m_rProfileName = rProfileName.c_str();
}

void CCreateLogsDlg::SetDefaults()
{
	ClearLogList();
	m_rLogItemHandler.DeleteLogs();
	m_logWidths.SetValue(1);
	m_logTypes.SetValue(1);
	m_dTotalHeight = 0;
	UpdateTotalHeight();
}

// returns 0 if no duplicates/same name found
int CCreateLogsDlg::GetProfileDuplicateCount()
{
	bool	bDuplicatesFound = false;
	std::string rProfileName = (const char *)m_txtProfileName.GetText().ToCStr();

	// remove dublicate number "(x)"
	rProfileName = rProfileName.substr(0, rProfileName.find('('));

	//int nProfileCountResult = 0;
	short nProfileCount = m_profileList.GetItemCount();

	std::vector<int> vecDuplicateNums = { 0 };

	for (short i = 1; i <= nProfileCount; i++)
	{
		int nNum = -1;

		std::string rCurrProfileName = (const char *)m_profileList.GetTabItemText(i, 1).ToCStr();

		size_t uIndxOfNumBeg = rCurrProfileName.find('(');

		std::string rCurrProfileNameSubst = rCurrProfileName.substr(0, rCurrProfileName.find('('));

		if (rProfileName == rCurrProfileNameSubst)
		{
			bDuplicatesFound = true;

			if (uIndxOfNumBeg != rCurrProfileName.length())
			{
				size_t uIndxOfNumEnd = rCurrProfileName.find(')');

				// find returns last index if not found
				if (uIndxOfNumEnd > uIndxOfNumBeg)
				{
					std::string		strNum = rCurrProfileName.substr(uIndxOfNumBeg + 1, uIndxOfNumEnd - uIndxOfNumBeg - 1);
					nNum = std::atoi(strNum.c_str());
				}
			}

			if (nNum != -1)
			{
				vecDuplicateNums.push_back(nNum);
			}
		}
	}

	std::sort(vecDuplicateNums.begin(), vecDuplicateNums.end());

	int nCurrNum = 0;

	if (bDuplicatesFound)
	{
		size_t i = 0;

		for (; i < vecDuplicateNums.size(); i++)
		{
			nCurrNum = vecDuplicateNums.at(i);

			if (i < vecDuplicateNums.at(i)) break;
		}

		if (i < vecDuplicateNums.size())
		{
			nCurrNum = (int)i;
		}
		else
		{
			nCurrNum++;
		}
	}

	return nCurrNum;
}

void CCreateLogsDlg::GetSelectedProfile(GS::UniString &rDestProfileName)
{
	rDestProfileName.Clear();

	short nSelectedItem = m_profileList.GetSelectedItem();

	// no select
	if (nSelectedItem == 0) return;

	rDestProfileName = m_profileList.GetTabItemText(nSelectedItem, 1);
}

void CCreateLogsDlg::DeleteProfile()
{
	short nSelectedItem = m_profileList.GetSelectedItem();

	// no select
	if (nSelectedItem == 0) return;

	GS::UniString		rProfileName;

	GetSelectedProfile(rProfileName);

	std::string rTempName = (const char *)rProfileName.ToCStr();

	int nResponse = ph::AcadQueryMsgBox(MB_OKCANCEL, "Are you sure you want to delete profile: \"%s\"", rTempName.c_str());
	// TODO selected profile attribute may be already deleted by user

	// OK
	if (nResponse == 1)
	{
		// delete attribute 
		GSErrCode nErr = CAttributeHelpers::DeleteAttributeByName(rProfileName.ToCStr(), API_ProfileID);

		if (nErr == APIERR_DELETED || nErr == APIERR_BADNAME) nErr = NoError;

		if (nErr != NoError) ph::CAcadException::Throw(nErr, "Trying to get attributes failed");

		// delete list item 
		m_profileList.DeleteItem(nSelectedItem);

		DeleteLogs();
	}
}

void CCreateLogsDlg::DeleteLogs()
{
	m_rLogItemHandler.DeleteLogs();
	ClearLogList();
}

void CCreateLogsDlg::DeleteSelectedLog()
{
	// log index in vector is always selected item value - 1
	short nLogToBeDeletedIndx = m_logList.GetSelectedItem();

	auto prLogItem = m_rLogItemHandler.GetLogById(nLogToBeDeletedIndx);

	if (prLogItem)
	{
		m_dTotalHeight -= prLogItem->GetType()->GetHeight();

		if (m_rLogItemHandler.DeleteLog(nLogToBeDeletedIndx))
		{
			UpdateLogList();
			UpdateTotalHeight();
		}
		else
		{
			ph::CAcadException::Throw(Error, "Trying to delete log failed.");
		}
	}
}

void CCreateLogsDlg::ClearLogList()
{
	m_logList.DeleteItem(DG::ListBox::AllItems);
	m_dTotalHeight = 0;
}

void CCreateLogsDlg::DrawLogList(short nSelectedLog)
{
	const CLogType	*prCurrLogType = nullptr;
	CLogItem		*prCurrLog = nullptr;
	short			nSelect = 1;
	char			buf[256], buf2[256];

	if (m_rLogItemHandler.GetCount() == 0) return;

	for (size_t i = 0; i < m_rLogItemHandler.GetCount(); i++)
	{
		prCurrLog = m_rLogItemHandler.GetLogByVecIndx(i);

		assert(prCurrLog);

		if (prCurrLog)
		{
			prCurrLogType = prCurrLog->GetType();

			assert(prCurrLogType);

			if (prCurrLogType)
			{
				// keep selection on selected log
				if (prCurrLog->GetId() == nSelectedLog)
				{
					nSelect = static_cast<short>(i + 1);
				}

				prCurrLog->SetId(static_cast<short>(i + 1));

				m_logList.InsertItem(prCurrLog->GetId());

				ph::AcadNumToString(prCurrLog->GetRealRise(), DG_ET_LENGTH, buf2, sizeof(buf2), ph::EAcadNumPrefDimensions);
				_snprintf(buf, sizeof(buf), "%s #%d / %s", prCurrLogType->GetName().c_str(), (int)(m_rLogItemHandler.GetCount() - i), buf2);
				m_logList.SetTabItemText(prCurrLog->GetId(), 1, buf);

				m_dTotalHeight += prCurrLogType->GetHeight();
			}
		}
	}

	// keep selection on selected log
	m_logList.SelectItem(nSelect);

	// populate GUI based on this selected log
	prCurrLog = m_rLogItemHandler.GetLogById(nSelect);
	prCurrLogType = prCurrLog->GetType();

	m_logTypes.SetValue(prCurrLogType->GetId());
	m_logBottoms.SetValue(prCurrLog->GetBottomType());
}

void CCreateLogsDlg::ClearProfileList()
{
	m_profileList.DeleteItem(DG::ListBox::AllItems);
}

void CCreateLogsDlg::UpdateLogList(short nSelectedLog)
{
	ClearLogList();
	DrawLogList(nSelectedLog);
}

void CCreateLogsDlg::UpdateTotalHeight()
{
	m_dTotalHeight = 0;
	char sTotalHeightText[128];

	std::vector<const CLogType *> vecLogTypes;

	for (size_t i = 0; i < m_rLogItemHandler.GetCount(); i++)
	{
		auto prCurrLog = m_rLogItemHandler.GetLogByVecIndx(i);
		auto prCurrType = prCurrLog->GetType();

		assert(prCurrType);

		if (prCurrType)
		{
			if (prCurrLog->GetBottomType() - 1 != CLogMaterials::CMatInfo::ELogTypeNormal)
			{
				CLogType rLogTypeByBottom = CLogTypeHandler::GetLogTypeByNameAndBottomId(prCurrType->GetName(), prCurrLog->GetBottomType() - 1);

				if (rLogTypeByBottom.IsInited())
				{
					m_dTotalHeight += rLogTypeByBottom.GetHeight();
				}
				else
				{
					m_dTotalHeight += prCurrType->GetHeight();
				}
			}
			else
			{
				m_dTotalHeight += prCurrType->GetHeight();
			}		

			if (!VectorContainsValue(vecLogTypes, prCurrType))
			{
				vecLogTypes.push_back(prCurrType);
			}
		}
	}

	// overlap once per log type
	for (size_t i = 0; i < vecLogTypes.size(); i++)
	{
		m_dTotalHeight += vecLogTypes.at(i)->GetOverLap();
	}

	sprintf(sTotalHeightText, "Total height: %.3f m", m_dTotalHeight);

	m_totalHeight.SetText(sTotalHeightText);
}

void CCreateLogsDlg::UpdateLogListUI()
{
	CLogItem *prSelectedLog = m_rLogItemHandler.GetLogById(m_logList.GetSelectedItem());
	assert(prSelectedLog);

	if (prSelectedLog)
	{
		const CLogType *prType = prSelectedLog->GetType();
		assert(prType);

		if (prType)
		{
			UpdateLogWidth(prType->GetWidth());
			PopulateLogTypes();

			m_logTypes.SetValue(prType->GetId());
			m_logBottoms.SetValue(prSelectedLog->GetBottomType());
		}
	}
}

void CCreateLogsDlg::UpdateLogWidth(double dLogWidth)
{
	GS::Int32	nID = 0;

	for (size_t i = 0; i < m_rLogTypeHandler.GetWidthCount(); i++)
	{
		auto prWidth = m_rLogTypeHandler.GetWidthByIndx(i);

		if (fabs(prWidth->dValue - dLogWidth) < ph::MM2) {
			nID = prWidth->nID;
			break;
		}
	}

	if (nID != 0)
	{
		m_logWidths.SetValue(nID);
	}
}

void CCreateLogsDlg::PopulateLogTypes()
{
	const CLogType		*prType = nullptr;
	DGMiscUC257Filler	filler(0);
	int					firstSel = 1;

	auto nSelectedWidth = m_logWidths.GetValue();

	if (nSelectedWidth <= 0) ph::CAcadException::Throw(Error, "Log width not selected/invalid value");

	auto prLogWidth = m_rLogTypeHandler.GetWidthByID(nSelectedWidth);

	assert(prLogWidth);
	if (!prLogWidth) return;

	m_rLogTypeHandler.UpdateLogTypesByWidth(prLogWidth->dValue);

	for (size_t i = 0; i < m_rLogTypeHandler.GetCount(); i++)
	{
		prType = m_rLogTypeHandler.GetLogTypeByVecIndx(i);

		if (prType)
		{
			filler.SetItem(prType->GetId() - 1, prType->GetName().c_str());

			// items id will be indx + 1 for some reason, so GetId() - 1 == id
		}
	}

	filler.ApplyToControl(m_logTypes);
	DGMISC_SetUC257Value(m_logTypes, firstSel, filler.GetCount());
}

void CCreateLogsDlg::PopulateLogBottoms()
{
	DGMiscUC257Filler	filler(0);
	int					firstSel = 1;

	InitBottomTypes();

	for (auto iter = m_mapBottomTypes.begin(); iter != m_mapBottomTypes.end(); ++iter)
	{
		filler.SetItem(iter->first, iter->second.c_str());
	}

	filler.ApplyToControl(m_logBottoms);
	DGMISC_SetUC257Value(m_logBottoms, firstSel, filler.GetCount());
}

void CCreateLogsDlg::InitBottomTypes()
{
	m_mapBottomTypes.insert(std::make_pair(0, "Normal"));
	m_mapBottomTypes.insert(std::make_pair(1, "Flat bottom"));
	m_mapBottomTypes.insert(std::make_pair(2, "Upper half to bottom of the wall"));
	m_mapBottomTypes.insert(std::make_pair(3, "Lower half to top of the wall"));
}

void CCreateLogsDlg::DrawProfileList()
{
	const API_Attribute *prCurrAttr = nullptr;

	for (size_t i = 0; i < m_rProfileAttributeHandler.GetCount(); i++)
	{
		prCurrAttr = m_rProfileAttributeHandler.GetProfileByVecIndx(i);

		//PH: Tulee assert ainakin dialogin esille otossa: assert(!prCurrAttr);

		if (prCurrAttr)
		{
			m_profileList.InsertItem(static_cast<short>(i + 1));
			m_profileList.SetTabItemText(static_cast<short>(i + 1), 1, prCurrAttr->header.name);
		}
	}
}

bool CCreateLogsDlg::QueryForPendingChanges(const char *pName)
{
	bool bResult = false;

	// OK == 1, CANCEL == 2
	int nresponse = ph::AcadQueryMsgBox(MB_OKCANCEL, "Do you want to save pending changes?");

	// ok
	if (nresponse == 1)
	{
		Save(pName);
		bResult = true;
	}

	return bResult;
}



/*
* CLogWallAttrData
*/

bool CLogWallAttrData::FromAttr(const API_Attr_Head &rAttrHead)
{
	ph::CAcadElemData	rElemData;

	if (rElemData.LoadAttr(rAttrHead))
		return false;

	return FromElemData(rElemData);
}


bool CLogWallAttrData::FromWall(const API_WallType &elemWall)
{
	ph::CAcadAttribute	attr;

	if (!elemWall.profileAttr)
		return false;

	attr.m_rAttr.header.typeID = API_ProfileID;
	attr.m_rAttr.header.index = elemWall.profileAttr;
	if (ACAPI_Attribute_Get(&attr.m_rAttr))
		return false;

	return FromAttr(attr.m_rAttr.header);
}



bool CLogWallAttrData::FromElemData(ph::CAcadElemData &rElemData)
{
	m_vecLogNames.clear();
	m_vecProfileTypes.clear();

	const ph::CAcadElemData::SBlockHead		*prBlockHead = NULL;

	const void	*pTempData = NULL;

	size_t uLogCount = 0;

	// Profile names
	if ((pTempData = rElemData.GetBlock(ATTR_PROFILE_ID, &prBlockHead)) == NULL)
		return false;

	std::vector<char*> vecLogNamePtrs;
	char *pTempName = strdup((const char*)pTempData);

	ph::StrParseListVec(vecLogNamePtrs, pTempName, '\n');

	uLogCount = vecLogNamePtrs.size();

	for (size_t i = 0; i < uLogCount; i++)
		m_vecLogNames.push_back(vecLogNamePtrs.at(i));

	free(pTempName);

	// Profile bottom types
	if ((pTempData = rElemData.GetBlock(ATTR_BOTTOMS_ID, &prBlockHead)) == NULL || prBlockHead->nLen < uLogCount * sizeof(GS::Int32))
		return false;

	GS::Int32* prBottomTypes = (GS::Int32*)pTempData;

	for (size_t i = 0; i < uLogCount; i++)
		m_vecProfileTypes.push_back(prBottomTypes[i]);

	return true;
}



/*
* CLogAttributeHandler
*/
CProfileAttributeHandler::CProfileAttributeHandler()
{
	Init();
}
CProfileAttributeHandler::~CProfileAttributeHandler()
{

}

/*
* Finds all profile attributes with grProfilePrefix
*/
void CProfileAttributeHandler::Init()
{
	API_Attribute	rCurrProfileAttr;
	API_AttributeIndex	nAttributeCount;
	GSErrCode		nErr;

	BNZeroMemory(&rCurrProfileAttr, sizeof(API_Attribute));

	rCurrProfileAttr.header.typeID = API_ProfileID;

	nErr = ACAPI_Attribute_GetNum(API_ProfileID, &nAttributeCount);

	for (API_AttributeIndex i = 1; i <= nAttributeCount; i++) {
		rCurrProfileAttr.header.index = i;
		nErr = ACAPI_Attribute_Get(&rCurrProfileAttr);

		if (nErr == NoError)
		{
			std::string rProfileName = rCurrProfileAttr.header.name;

			size_t uFound = rProfileName.find(grProfilePrefix, 0);

			if (uFound != std::string::npos && rCurrProfileAttr.profile.wallType)
			{
				m_vecProfiles.push_back(rCurrProfileAttr);
			}

			continue;
		}

		if (nErr == APIERR_DELETED) nErr = NoError;

		if (nErr != NoError) ph::CAcadException::Throw(nErr, "Trying to get attributes failed");
	}
}

size_t CProfileAttributeHandler::GetCount()
{
	return m_vecProfiles.size();
}

const std::vector<API_Attribute> *CProfileAttributeHandler::GetProfileList()
{
	return &m_vecProfiles;
}

API_Attribute *CProfileAttributeHandler::GetProfileByVecIndx(size_t nVecIndx)
{
	if (nVecIndx >= m_vecProfiles.size()) throw new std::out_of_range("Index out of range.");

	auto profiles = m_vecProfiles.data();
	return &profiles[nVecIndx];
}

bool CProfileAttributeHandler::DeleteProfile(const std::string &rProfileName)
{
	for (size_t i = 0; i < m_vecProfiles.size(); i++)
	{
		if (m_vecProfiles[i].header.name == rProfileName)
		{
			m_vecProfiles.erase(m_vecProfiles.begin() + i);

			return true;
		}
	}

	return false;
}

GSErrCode CProfileAttributeHandler::SaveProfileData(API_Attr_Head &rAttrHead, CLogItemHandler &rLogsHandler)
{
	ph::CAcadElemData		rElemDataHandler;
	std::string				rLogNames;
	size_t					uLogCount = rLogsHandler.GetCount();

	if (uLogCount == 0) return NoError;

	GS::Int32				*prLogBottomTypes = new GS::Int32[uLogCount];

	for (GS::Int32 i = 0; i < rLogsHandler.GetCount(); i++)
	{
		auto prCurrLog = rLogsHandler.GetLogByVecIndx(i);

		assert(prCurrLog);

		auto rCurrLogName = prCurrLog->GetType()->GetName();

		assert(!rCurrLogName.empty());

		rLogNames.append(rCurrLogName);
		rLogNames.append("\n");
		prLogBottomTypes[i] = prCurrLog->GetBottomType();
	}

	auto size = rLogNames.size() + 1;
	auto prLogNames = (char*)malloc(size);

	if (prLogNames == nullptr) ph::CAcadException::Throw(Error, "Malloc failed.");

	strcpy(prLogNames, rLogNames.c_str());

	// TODO should be in same block?
	rElemDataHandler.SaveBlock(ATTR_PROFILE_ID, (int)size, 0, prLogNames);
	rElemDataHandler.SaveBlock(ATTR_BOTTOMS_ID, sizeof(*prLogBottomTypes) * (int)uLogCount, 0, prLogBottomTypes);

	GSErrCode nErr = rElemDataHandler.SaveAttr(rAttrHead);

	free(prLogNames);
	delete[] prLogBottomTypes;

	return nErr;
}


GSErrCode CProfileAttributeHandler::DeleteProfileData(API_Attr_Head &rAttrHead)
{
	ph::CAcadElemData		rElemDataHandler;

	rElemDataHandler.SaveBlock(ATTR_PROFILE_ID, 0, 0, NULL);
	rElemDataHandler.SaveBlock(ATTR_BOTTOMS_ID, 0, 0, NULL);

	GSErrCode nErr = rElemDataHandler.SaveAttr(rAttrHead);

	return nErr;
}



/*
* CLogItem
*/
CLogItem::CLogItem()
{
	m_nId = 0;
	m_nBottomType = 0;
	m_prType = nullptr;
}
CLogItem::~CLogItem()
{

}

CLogItem::CLogItem(short nId, GS::Int32 nBottomType, const CLogType *prLogType)
{
	assert(prLogType);
	m_nId = nId;
	m_nBottomType = nBottomType;
	m_prType = prLogType;
}

void CLogItem::SetId(short nId)
{
	m_nId = nId;
}

void CLogItem::SetBottomType(GS::Int32 nBottomType)
{
	m_nBottomType = nBottomType;
}

void CLogItem::SetType(const CLogType *prType)
{
	assert(prType);
	m_prType = prType;
}

short CLogItem::GetId() const
{
	return m_nId;
}

GS::Int32 CLogItem::GetBottomType() const
{
	return m_nBottomType;
}

const CLogType *CLogItem::GetType() const
{
	assert(m_prType);
	return m_prType;
}

double CLogItem::GetRealRise() const
{
	const CLogMaterials::CMatInfo	*pMat;
	const CLogType	*pType = GetType();

	if (!pType)
	{
		assert(false);
		return 0;
	}

	if ((pMat = grLogMaterials.FindByName(pType->GetName().c_str())) == NULL)
		return 0.0;

	double	res = pMat->m_dHeight;

	switch (m_nBottomType)
	{
	case CLogMaterials::CMatInfo::ELogTypeUpperHalf + 1:
		res = pMat->m_dHeight * 0.5 - pMat->m_dHalfingOffUpper;			// Negative moves lower
		break;

	case CLogMaterials::CMatInfo::ELogTypeLowerHalf + 1:
		res = pMat->m_dHeight * 0.5 + pMat->m_dHalfingOffUpper;
		break;
	}

	return res;
}



/*
* CLogItemHandler
*/
size_t CLogItemHandler::GetCount() const
{
	return m_vecLogs.size();
}

const std::vector<CLogItem> *CLogItemHandler::GetLogList() const
{
	return &m_vecLogs;
}

void CLogItemHandler::AddLog(const CLogItem &rLog)
{
	assert(rLog.GetType());

	if (rLog.GetId() == 0)
	{
		m_vecLogs.push_back(rLog);
	}
	else
	{
		m_vecLogs.insert(m_vecLogs.begin() + (rLog.GetId() - 1), rLog);
	}
}

CLogItem *CLogItemHandler::GetLogByVecIndx(size_t nVecIndx)
{
	if (nVecIndx >= m_vecLogs.size()) throw new std::out_of_range("Index out of range.");

	auto logs = m_vecLogs.data();
	return &logs[nVecIndx];
}

CLogItem *CLogItemHandler::GetLogById(short nLogId)
{
	if (nLogId < 0 || nLogId > m_vecLogs.size()) throw new std::out_of_range("Index out of range.");

	CLogItem *prLog = nullptr;

	for (size_t i = 0; i < GetCount(); i++)
	{
		if (m_vecLogs[i].GetId() == nLogId)
		{
			auto logs = m_vecLogs.data();
			prLog = &logs[i];
			break;
		}
	}

	return prLog;
}

bool CLogItemHandler::DeleteLog(size_t uLogIndx)
{
	for (size_t i = 0; i < m_vecLogs.size(); i++)
	{
		if (m_vecLogs[i].GetId() == uLogIndx)
		{
			m_vecLogs.erase(m_vecLogs.begin() + i);

			return true;
		}
	}

	return false;
}

void CLogItemHandler::DeleteLogs()
{
	m_vecLogs.clear();
}

bool CLogItemHandler::MoveLog(bool bMoveUp, short nSelectedLogId)
{
	bool	bLogFound = false;
	int		nPos = 0;
	size_t	nLogCount = GetCount();

	// Nothing selected
	if (nSelectedLogId == 0) return true;

	// get previous log settings
	for (size_t i = 0; i < nLogCount; i++)
	{
		if (m_vecLogs[i].GetId() == nSelectedLogId)
		{
			if (bMoveUp)
			{
				nPos = static_cast<int>(i - 1);

				// log is already topmost 
				if (nPos >= 0)
				{
					std::iter_swap(m_vecLogs.begin() + i, m_vecLogs.begin() + nPos);
				}
			}
			else
			{
				nPos = static_cast<int>(i + 1);

				// log is already bottom-most
				if (nPos < nLogCount)
				{
					std::iter_swap(m_vecLogs.begin() + i, m_vecLogs.begin() + nPos);
				}
			}

			bLogFound = true;
			break;
		}
	}

	return bLogFound;
}




/*
* CLogTypeHandler
*/

CLogTypeHandler::CLogTypeHandler()
{
	Init();
}

bool CLogTypeHandler::IsInited()
{
	return m_bIsInited;
}

const CLogType *CLogTypeHandler::GetLogTypeById(GS::Int32 nLogTypeId) const
{
	for (size_t i = 0; i < m_vecLogTypesByWidth.size(); i++)
	{
		if (m_vecLogTypesByWidth[i]->GetId() == nLogTypeId)
		{
			auto prLogTypeFound = GetLogTypeByName(m_vecLogTypesByWidth[i]->GetName());

			return prLogTypeFound;
		}
	}

	return nullptr;
}

const CLogType *CLogTypeHandler::GetLogTypeByVecIndx(size_t uVecIndx) const
{
	if (uVecIndx >= m_vecLogTypesByWidth.size()) throw new std::out_of_range("Index out of range");

	return m_vecLogTypesByWidth[uVecIndx];
}

const CLogType *CLogTypeHandler::GetLogTypeByName(const std::string &rLogName)	const
{
	for (size_t i = 0; i < m_vecLogTypes.size(); i++)
	{
		if (m_vecLogTypes[i].GetName() == rLogName)
		{
			auto logs = m_vecLogTypes.data();
			return &logs[i];
		}
	}

	return nullptr;
}

void CLogTypeHandler::Init()
{
	m_vecLogTypes.clear();

	auto &vecMat = grLogMaterials.GetMatVec();

	for (size_t i = 0; i < vecMat.size(); i++)
	{
		CLogType rCurrLogType;

		auto &rCurrMat = vecMat.at(i);

		/*
		*	ELogTypeNormal,			// <profile> in xml
		*	ELogTypeLowestFull,		// <profilelowest> in xml
		*	ELogTypeUpperHalf,		// Lowest half log in the wall
		*	ELogTypeLowerHalf
		*/

		if (rCurrMat.GetLogType(rCurrLogType))
		{
			if (rCurrLogType.IsInited())
			{
				rCurrLogType.SetId(static_cast<GS::Int32>(i + 1));

				m_vecLogTypes.push_back(rCurrLogType);
			}
		}
	}

	if (!m_vecLogTypes.empty())
	{
		InitLogWidths();
	}

	m_bIsInited = !m_vecLogTypes.empty();
}

void CLogTypeHandler::InitLogWidths()
{
	m_vecWidths.clear();

	for (size_t i = 0; i < m_vecLogTypes.size(); i++)
	{
		auto rCurrType = m_vecLogTypes.at(i);

		SWidthType rCurrWidth;

		rCurrWidth.dValue = rCurrType.GetWidth();
		size_t j = 0;

		for (; j < m_vecWidths.size(); j++)
		{
			if (fabs(m_vecWidths.at(j).dValue - rCurrWidth.dValue) < ph::MM2)
			{
				break;
			}
		}

		// width not found
		if (j == m_vecWidths.size())
		{
			m_vecWidths.push_back(rCurrWidth);
		}
	}

	std::sort(m_vecWidths.begin(), m_vecWidths.end(), Sort);

	// ugly
	for (size_t i = 0; i < m_vecWidths.size(); i++)
	{
		m_vecWidths.at(i).nID = (GS::Int32) (i + 1);
	}
}


bool CLogTypeHandler::UpdateLogTypesByWidth(double dSelectedWidth)
{
	GS::Int32 nLogTypeID = 0;

	m_vecLogTypesByWidth.clear();

	for (size_t i = 0; i < m_vecLogTypes.size(); i++)
	{
		CLogType &rCurrType = m_vecLogTypes.at(i);

		if (fabs(rCurrType.GetWidth() - dSelectedWidth) < ph::MM2)
		{
			nLogTypeID++;

			rCurrType.SetId(nLogTypeID);

			m_vecLogTypesByWidth.push_back(&rCurrType);
		}
	}

	return !m_vecLogTypesByWidth.empty();
}

size_t CLogTypeHandler::GetCount()
{
	return m_vecLogTypesByWidth.size();
}

size_t CLogTypeHandler::GetWidthCount()
{
	return m_vecWidths.size();
}

const CLogTypeHandler::SWidthType *CLogTypeHandler::GetWidthByIndx(size_t uIndx)
{
	return &m_vecWidths.at(uIndx);
}

const CLogTypeHandler::SWidthType *CLogTypeHandler::GetWidthByID(GS::Int32 nID)
{
	for (size_t i = 0; i < m_vecWidths.size(); i++)
	{
		auto prCurrWidth = &m_vecWidths.at(i);

		if (prCurrWidth->nID == nID) return prCurrWidth;
	}

	return nullptr;
}

/*
*
*
*  Profile wall handler
*
*
*/
CLogWallHandler::CLogWallHandler()
{

}
CLogWallHandler::~CLogWallHandler()
{

}

void CLogWallHandler::BuildProfileAttribute(const std::vector<CLogItem> &vecLogItems, const std::string &rProfileName, API_Attribute &rDestAttr, API_AttributeDefExt &rProfileAttrDef)
{
	BNZeroMemory((GSPtr)&rDestAttr, sizeof(API_Attribute));
	BNZeroMemory((GSPtr)&rProfileAttrDef, sizeof(API_AttributeDefExt));

	rDestAttr.header.typeID = API_ProfileID;

	strcpy(rDestAttr.header.name, rProfileName.c_str());

	rDestAttr.profile.wallType = true;
	rDestAttr.profile.beamType = false;
	rDestAttr.profile.coluType = false;

	rProfileAttrDef.profile_vectorImageItems = new ProfileVectorImage();

	double dHeightOffset = 0;

	for (int i = static_cast<int>(vecLogItems.size() - 1); i >= 0; i--)
	{
		auto &rCurrLog = vecLogItems.at(i);
		auto prCurrType = rCurrLog.GetType();

		if (rCurrLog.GetBottomType() - 1 != CLogMaterials::CMatInfo::ELogTypeNormal)
		{
			// aoifhgoegbsolbsdfbsiodfb YÖK!!!!!!!!!!!!
			CLogType rLogTypeByBottomType = CLogTypeHandler::GetLogTypeByNameAndBottomId(prCurrType->GetName(), rCurrLog.GetBottomType() - 1);

			if (rLogTypeByBottomType.IsInited())
			{
				BuildProfileImage(rProfileAttrDef.profile_vectorImageItems, &rLogTypeByBottomType, dHeightOffset);

				dHeightOffset += rLogTypeByBottomType.GetHeight();
			}
		}
		else
		{
			BuildProfileImage(rProfileAttrDef.profile_vectorImageItems, prCurrType, dHeightOffset);
			dHeightOffset += prCurrType->GetHeight();
		}
	}
}

GSErrCode CLogWallHandler::CreateProfile(API_Attribute &rProfileAttr, API_AttributeDefExt &rProfileAttrDef)
{
	GSErrCode nErr = ACAPI_Attribute_CreateExt(&rProfileAttr, &rProfileAttrDef);

	if (nErr != APIERR_ATTREXIST)
	{
		ACAPI_DisposeAttrDefsHdlsExt(&rProfileAttrDef);
	}

	return nErr;
}

GSErrCode CLogWallHandler::UpdateProfile(API_Attribute &rProfileAttr, API_AttributeDefExt &rProfileAttrDef)
{
	GSErrCode nErr = ACAPI_Attribute_ModifyExt(&rProfileAttr, &rProfileAttrDef);

	ACAPI_DisposeAttrDefsHdlsExt(&rProfileAttrDef);

	return nErr;
}


void CLogWallHandler::ApplyLogProfileToSelectedWalls(const API_Attribute &rAttr)
{
	ph::CElemListHolder		rElems;
	API_Element				rCurrWall, rMask;
	GSErrCode				nErr;

	if (!rAttr.profile.wallType) return;

	nErr = rElems.GetSelected(API_WallID);

	if (nErr == APIERR_NOSEL)
	{
		return;
	}
	else if (nErr != NoError)
	{
		ph::CAcadException::Throw(nErr, "Trying to get selected walls failed.");
	}

	size_t uSelectedElemCount = rElems.GetCount();

	for (size_t i = 0; i < uSelectedElemCount; i++)
	{
		BNZeroMemory(&rCurrWall, sizeof(rCurrWall));

		if ((nErr = rElems.GetElement(i, &rCurrWall)) != NoError) ph::CAcadException::Throw(nErr, "Trying to get element failed.");

		ACAPI_ELEMENT_MASK_CLEAR(rMask);
		ACAPI_ELEMENT_MASK_SET(rMask, API_WallType, profileAttr);
		ACAPI_ELEMENT_MASK_SET(rMask, API_WallType, modelElemStructureType);
		ACAPI_ELEMENT_MASK_SET(rMask, API_WallType, materialsChained);
		ACAPI_ELEMENT_MASK_SET(rMask, API_WallType, refMat);
		ACAPI_ELEMENT_MASK_SET(rMask, API_WallType, oppMat);
		ACAPI_ELEMENT_MASK_SET(rMask, API_WallType, sidMat);

		rCurrWall.wall.profileAttr = rAttr.profile.head.index;
		rCurrWall.wall.modelElemStructureType = API_ProfileStructure;

		rCurrWall.wall.refMat.overridden = true;
		rCurrWall.wall.oppMat.overridden = true;
		rCurrWall.wall.sidMat.overridden = true;

		if ((nErr = ACAPI_Element_Change(&rCurrWall, &rMask, nullptr, 0, true)) != NoError) ph::CAcadException::Throw(nErr, "Trying to edit wall failed.");

		if ((nErr = ph::AcadAttachObserver(&rCurrWall.header, 0)) != NoError && nErr != APIERR_LINKEXIST) ph::CAcadException::Throw(nErr, "Attaching observer failed.");
	}
}

// -----------------------------------------------------------------------------
//  Creates the internal structure of a profile attribute
// -----------------------------------------------------------------------------

void CLogWallHandler::BuildProfileImage(ProfileVectorImage *pVectorImage, const CLogType *prLogType, double dHeightOffset)
{
	assert(prLogType);
	if (!prLogType)
		return;

	CApiPolygon			rPoly = prLogType->m_rPoly;
	std::vector<Coord>	vecCoords;
	std::vector<double>	vecArcAngles;
	std::vector<void*>	vecPtData;

	rPoly.Offset(0.0, dHeightOffset);			// No need for EndPoly()
	rPoly.GetForHatchObject(vecCoords, vecArcAngles, vecPtData);
	assert(vecCoords.size() == vecArcAngles.size());

	short			nCoords = static_cast<short>(vecCoords.size() - 1);			// Contains [0]=0,0 (not real coordinate)
	Int32			size = sizeof(ProfileItem) + (nCoords + 1) * sizeof(ProfileEdgeData);
	GSHandle		addInfo = BMAllocateHandle(size, ALLOCATE_CLEAR, 0);
	UInt32			boends[2] = { 0, (UInt32) nCoords };

	if (!DBERROR(addInfo == nullptr)) {
		BNZeroMemory(*addInfo, size);

		ProfileItem	*pProfileItem = reinterpret_cast<ProfileItem*>(*addInfo);
		pProfileItem->obsoletePriorityValue = 0;		// not used
		pProfileItem->profileItemVersion = ProfileItemVersion;
		pProfileItem->SetCutEndLinePen(1);
		pProfileItem->SetCutEndLineType(1);
		pProfileItem->SetVisibleCutEndLines(true);
		pProfileItem->SetCore(true);

		ProfileEdgeData *pProfileEdgeData = reinterpret_cast<ProfileEdgeData*>(reinterpret_cast<char*>(*addInfo) + sizeof(ProfileItem));

		// PH: Oisko tämä käyttämättä?
		pProfileEdgeData[0].SetPen(1);
		pProfileEdgeData[0].SetLineType(1);
		pProfileEdgeData[0].SetMaterial(1);
		pProfileEdgeData[0].SetFlags(ProfileEdgeData::IsVisibleLineFlag);

		for (short i = 1; i <= nCoords; i++) {
			pProfileEdgeData[i].SetPen(1);
			pProfileEdgeData[i].SetLineType(1);
			pProfileEdgeData[i].SetMaterial(1);

			// TODO: Tässä kaivetaan xml:ssä annettu maskikoodi ja pitäisi olla näkymätön sivu, jos gdlMask==79
			int	gdlMask = (int)(GS::IntPtr) vecPtData.at(i);

			gdlMask = gdlMask % 100;
			pProfileEdgeData[i].SetFlags((gdlMask == 79) ? ProfileEdgeData::IsCurvedFlag : ProfileEdgeData::IsVisibleLineFlag);
		}
	}

	GX::Pattern::HatchTran	hatchTrafo;
	Sy_HatchType			hatchType;
	HatchObject				hatchObject;
	PlaneEq					plane;
	//GS_RGBColor				color = { 0, 0, 0 };

	hatchTrafo.SetGlobal();

	plane.pa = 0;
	plane.pb = 0;
	plane.pc = 0;
	plane.pd = 0;

	hatchType.item_Typ = SyHatch;

	VBAttr::ExtendedPen	contPen(1);

	//hatchObject.syFlags = 0;
	//hatchObject.buildMatFlags = SyHatchFlag_FillHatch | SyHatchFlag_OverrideBkgPen | SyHatchFlag_OverrideFgPen;		//  SyHatchFlag_BuildingMaterialHatch;
	//hatchObject.fillIdx = 0;
	//hatchObject.buildMatIdx = 1;
	//hatchObject.fillBkgPen = 0;
	//hatchObject.determine = 0;
	//hatchObject.specFor3D = 0;
	//hatchObject.sy2dRenovationStatus = 0;
	//hatchObject.bkgColorRGB = color;
	//hatchObject.fgColorRGB = color;
	//hatchObject.SetDisplayOrder(1);
	//hatchObject.origPlane = plane;
	hatchObject.hatchTrafo = hatchTrafo;
	hatchObject.SetAddInfo(addInfo);

	if (hatchObject.CheckState())
		HatchObject::Create(hatchType, hatchObject, true, contPen, 1, 0, DrwIndexForHatches, hatchTrafo, 1, boends, nCoords, vecCoords.data(), vecArcAngles.data(), plane, 1, 0, API_DefaultStatus, 0);

	/*
	*	Does not show the contour line - could not find a way to do that:
	*
	hatchObject.SetContLType(1);
	hatchObject.SetContPen(contPen);
	hatchObject.SetContVis(true);
	hatchObject.GetContLType();
	contPen = hatchObject.GetContPen();
	hatchObject.GetContVis();
	*/

	GS::GSErrCode nErr = pVectorImage->AddHatch(hatchType, hatchObject);

	if ((nErr = pVectorImage->CheckAndRepair()) != 0) {
		if (addInfo != nullptr)
			BMKillHandle(&addInfo);

		ph::CAcadException::Throw(nErr, "Building profile image failed.");
	}

	if (addInfo != nullptr)
	{
		BMKillHandle(&addInfo);
	}
}

//// -----------------------------------------------------------------------------
////  List one profile
//// -----------------------------------------------------------------------------
//void CLogWallHandler::ListProfileDescription(const ProfileVectorImage& profileDescription, const GS::HashTable<PVI::ProfileParameterId, GS::UniString>& parameterNameTable)
//{
//	//profileDescription.ReadXML
//	ConstProfileVectorImageIterator profileDescriptionIt(profileDescription);
//	const GS::HashTable<GS::Guid, PVI::HatchVertexId>& associatedHotspotsTable = profileDescription.GetAssociatedHotspotsTable();
//
//	while (!profileDescriptionIt.IsEOI()) {
//		switch (profileDescriptionIt->item_Typ) {
//		case SyHots: {
//			const Sy_HotType* pSyHot = ((const Sy_HotType*)profileDescriptionIt);
//			//WriteReport("- Hotspot (id: %s)", pSyHot->GetUniqueId().ToUniString().ToCStr().Get());
//			//WriteReport("-- Coordinate: %.3f, %.3f", pSyHot->c.x, pSyHot->c.y);
//			if (associatedHotspotsTable.ContainsKey(pSyHot->GetUniqueId())) {
//				const PVI::HatchVertexId associatedVertexId = associatedHotspotsTable[pSyHot->GetUniqueId()];
//				//WriteReport("-- Hotspot is associated to hatch %s's %d. vertex.", associatedVertexId.GetHatchId().ToUniString().ToCStr().Get(), associatedVertexId.GetVertexIndex());
//			}
//		}
//					 break;
//
//		case SyLine: {
//			const Sy_LinType* pSyLine = static_cast <const Sy_LinType*> (profileDescriptionIt);
//			//WriteReport("- Line");
//			//WriteReport("-- Layer: %d; SpecForProfile: %d", pSyLine->sy_fragmentIdx, pSyLine->specForProfile);
//			//WriteReport("-- From: %.3f, %.3f", pSyLine->begC.x, pSyLine->begC.y);
//			//WriteReport("-- To  : %.3f, %.3f", pSyLine->endC.x, pSyLine->endC.y);
//		}
//					 break;
//
//		case SyHatch: {
//			const HatchObject& syHatch = profileDescriptionIt;
//			const ProfileItem* profileItemInfo = syHatch.GetProfileItemPtr();
//
//			//auto classInfo = syHatch.
//			ph::CAcadLogWnd::AddTextFormat("- Hatch (id: %s, componentIndex: %d)", syHatch.GetUniqueId().ToUniString().ToCStr().Get(), profileDescription.GetComponentId(syHatch.GetUniqueId()));
//			Int32 uiPriority = 0;
//			{
//				API_BuildingMaterialType	buildMat;
//				BNZeroMemory(&buildMat, sizeof(API_BuildingMaterialType));
//				buildMat.head.typeID = API_BuildingMaterialID;
//				buildMat.head.index = syHatch.buildMatIdx;
//				ACAPI_Attribute_Get((API_Attribute*)&buildMat);
//				ACAPI_Goodies(APIAny_Elem2UIPriorityID, (void*)&buildMat.connPriority, &uiPriority);
//			}
//			ph::CAcadLogWnd::AddTextFormat("-- Priority: %d%s", uiPriority, profileItemInfo->IsCore() ? ", Core component" : "");
//			ph::CAcadLogWnd::AddTextFormat("-- Coordinates: %ld", syHatch.GetCoords().GetSize());
//			UInt32 index = 0;
//			for (Coord coord : syHatch.GetCoords()) {
//				const bool isContourEnd = index != 0 && syHatch.GetSubPolyEnds().Contains(index);
//				ph::CAcadLogWnd::AddTextFormat("--- Coord #%-2d (%5.2f, %5.2f)%s", index++, coord.x, coord.y, isContourEnd ? " - Contour end" : "");
//			}
//
//			const ProfileEdgeData*	profileEdgeData = syHatch.GetProfileEdgeDataPtr();
//			for (UInt32 ii = 0; ii < syHatch.GetCoords().GetSize(); ii++, profileEdgeData++) {
//				ph::CAcadLogWnd::AddTextFormat("--- Edge data #%-2d surface = %32s (%3ld) pen = (%3ld)", ii, CAttributeHelpers::GetAttributeName(API_MaterialID, profileEdgeData->GetMaterial()),
//					profileEdgeData->GetMaterial(), profileEdgeData->GetPen());
//			}
//		}
//					  break;
//
//		case SySpline: {
//			const Sy_SplineType* pSySpline = static_cast <const Sy_SplineType*> (profileDescriptionIt);
//			const API_Coord* pSyCoords = (const API_Coord*)((const char*)pSySpline + pSySpline->coorOff);
//			const Geometry::DirType* pSyDirs = (const Geometry::DirType*) ((const char*)pSySpline + pSySpline->dirsOff);
//			for (GSIndex ii = 0; ii < pSySpline->nCoords; ++ii) {
//				ph::CAcadLogWnd::AddTextFormat("--- Coord #%-2d (%5.2f, %5.2f)", ii, pSyCoords[ii].x, pSyCoords[ii].y);
//				ph::CAcadLogWnd::AddTextFormat("--- Dirs  #%-2d (%5.2f, %5.2f, %5.2f)", ii, pSyDirs[ii].lenPrev, pSyDirs[ii].lenNext, pSyDirs[ii].dirAng);
//			}
//		}
//					   break;
//
//		case SyArc: {
//			const Sy_ArcType* pSyArc = static_cast <const Sy_ArcType*> (profileDescriptionIt);
//			//WriteReport("- Arc");
//			//WriteReport("-- Layer		: %d", pSyArc->sy_fragmentIdx);
//			//WriteReport("-- Origo		: %.3f, %.3f", pSyArc->origC.x, pSyArc->origC.y);
//			//WriteReport("-- BegCoord	: %.3f, %.3f", pSyArc->begC.x, pSyArc->begC.y);
//			//WriteReport("-- EndCoord	: %.3f, %.3f", pSyArc->endC.x, pSyArc->endC.y);
//			//WriteReport("-- Radius		: %.3f", pSyArc->r);
//		}
//					break;
//
//		case SyPolyLine:
//		case SyText:
//		case SyPicture:
//		case SyPixMap:
//		case SyPointCloud:
//			break;
//		}
//		++profileDescriptionIt;
//	}
//
//	for (auto itPar = profileDescription.GetParameterTable().EnumeratePairs(); itPar != nullptr; ++itPar) {
//		const PVI::ProfileParameterId& parameterId = *itPar->key;
//		const GS::HashSet<PVI::ProfileParameterSetupId>& parDefIDs = *itPar->value;
//		const GS::UniString parameterName = parameterNameTable[parameterId];
//
//		for (auto it = parDefIDs.Enumerate(); it != nullptr; ++it) {
//			const PVI::ProfileParameterSetupId& parameterDefId = *it;
//			const PVI::ProfileVectorImageParameter& profileParameter = profileDescription.GetParameterDef(parameterDefId);
//
//			//WriteReport("- ProfileParameter (id: %s, name: %s)", parameterId.ToUniString().ToCStr().Get(), parameterName.ToCStr().Get());
//			if (profileParameter.GetType() == PVI::ProfileVectorImageParameter::ParameterType::EdgeOffset) {
//				const PVI::EdgeOffsetParameter& edgeOffsetParameter = profileParameter.GetEdgeOffsetParameter();
//
//				const PVI::ProfileDimControlToolId& dimToolID = edgeOffsetParameter.GetDimControlToolID();
//				const GS::Optional<PVI::DimensionControlTool> dimTool = profileDescription.GetDimensionControlTool(dimToolID);
//				DBASSERT(dimTool.HasValue());
//				const PVI::ProfileAnchorId& begAnchorID = dimTool->GetBegAnchorID();
//				const PVI::ProfileAnchorId& endAnchorID = dimTool->GetEndAnchorID();
//				const GS::Optional<PVI::Anchor>	begAnchor = profileDescription.GetAnchor(begAnchorID);
//				const GS::Optional<PVI::Anchor>	endAnchor = profileDescription.GetAnchor(endAnchorID);
//				DBASSERT(begAnchor.HasValue());
//				DBASSERT(endAnchor.HasValue());
//
//				//TODO B530M2 AKARKINEK: M1 agon a begAnchor a szokasos vertex vagy fix pontos, mig az endAnchor a main edge... M2 agon ez megvaltozik!
//				PVI::HatchEdgeId mainEdge = endAnchor->GetAssociatedEdgeId();
//				//WriteReport("-- MainEdge: (HatchId: %s, EdgeIndex: %d, Direction: %s)", mainEdge.GetHatchId().ToUniString().ToCStr().Get(),
//				//	mainEdge.GetEdgeIndex(),
//				//	ProfileVectorImageOperations::ResolveEdgeDirFlag(profileDescription, mainEdge) == PVI::AssociatedEdge::DirectionFlag::Left ? "Left" : "Right");
//				for (const PVI::AssociatedEdge& associatedEdge : edgeOffsetParameter.GetAssociatedEdges()) {
//					//WriteReport("-- Further edges to offset: (HatchId: %s, EdgeIndex: %d, Direction: %s)", associatedEdge.GetHatchId().ToUniString().ToCStr().Get(),
//					//	associatedEdge.GetEdgeIndex(),
//					//	associatedEdge.GetDirectionFlag() == PVI::AssociatedEdge::DirectionFlag::Left //TODO B530M2!!!
//					//	? "Left" : "Right");
//				}
//				const PVI::Anchor& anchor = begAnchor.Get(); //TODO B530M2!!!
//
//				if (anchor.GetAnchorType() == PVI::Anchor::AnchorType::VertexAssociative) {
//					//WriteReport("-- Anchor is on vertex. (HatchId: %s, VertexIndex: %d", anchor.GetAssociatedVertexId().GetHatchId().ToUniString().ToCStr().Get(),
//					//	anchor.GetAssociatedVertexId().GetVertexIndex());
//				}
//				else if (anchor.GetAnchorType() == PVI::Anchor::AnchorType::FixedToStretchCanvas) {
//					//WriteReport("-- Anchor is fixed. (%.3f, %.3f)", anchor.GetFixAnchorPosition().GetX(), anchor.GetFixAnchorPosition().GetY());
//				}
//			}
//			else {
//				// other types are not supported.
//			}
//		}
//	}
//
//	{
//		const PVI::StretchData& stretchData = profileDescription.GetStretchData();
//		if (stretchData.HasHorizontalZone()) {
//			//WriteReport("- Horizontal Stretch Zone: (%.3f, %.3f)", stretchData.GetHorizontalZoneMin(), stretchData.GetHorizontalZoneMax());
//		}
//		if (stretchData.HasVerticalZone()) {
//			//WriteReport("- Vertical Stretch Zone: (%.3f, %.3f)", stretchData.GetVerticalZoneMin(), stretchData.GetVerticalZoneMax());
//		}
//	}
//
//	{
//		const PVI::EdgeOverrideData& edgeOverrideData = profileDescription.GetEdgeOverrideData();
//		if (edgeOverrideData.GetOverrideSectionLines()) {
//			//WriteReport("- Inner section lines overridden: LineType to %s, linePen to %s", AttributeName(API_LinetypeID, edgeOverrideData.GetInnerLineType()), AttributeName(API_PenID, edgeOverrideData.GetInnerPen()));
//			//WriteReport("- Outer section lines overridden: LineType to %s, linePen to %s", AttributeName(API_LinetypeID, edgeOverrideData.GetOuterLineType()), AttributeName(API_PenID, edgeOverrideData.GetOuterPen()));
//		}
//	}
//
//}

//GSErrCode CLogWallHandler::Do_ReadProfileWall()
//{
//	GS::Array<API_Guid>	inds;
//	GSErrCode            err;
//	API_SelectionInfo    selectionInfo;
//	API_Element          tElem;
//	API_Neig             **selNeigs;
//
//	err = ACAPI_Selection_Get(&selectionInfo, &selNeigs, true);
//	BMKillHandle((GSHandle *)&selectionInfo.marquee.coords);
//	if (err == APIERR_NOSEL)
//		err = NoError;
//
//	if (err != NoError) {
//		BMKillHandle((GSHandle *)&selNeigs);
//		return err;
//	}
//
//	if (selectionInfo.typeID != API_SelEmpty) {
//		// collect indexes of selected dimensions
//		UInt32 nSel = BMGetHandleSize((GSHandle)selNeigs) / sizeof(API_Neig);
//		for (UInt32 ii = 0; ii < nSel && err == NoError; ++ii) {
//			tElem.header.typeID = ph::AcadNeigToElemID((*selNeigs)[ii].neigID);
//			if (tElem.header.typeID != API_WallID)
//				continue;
//
//			if (!ACAPI_Element_Filter((*selNeigs)[ii].guid, APIFilt_IsEditable))
//				continue;
//
//			tElem.header.guid = (*selNeigs)[ii].guid;
//			if (ACAPI_Element_Get(&tElem) != NoError)
//				continue;
//
//			// Add wall to the array
//			inds.Push(tElem.header.guid);
//		}
//	}
//
//	if (inds.GetSize() > 0)
//	{
//		API_Element rElem, rMask;
//		API_Attribute	attrib;
//		API_AttributeDefExt	defs;
//		//API_ElementMemo	rMemo;
//
//		BNZeroMemory(&rElem, sizeof(rElem));
//
//		rElem.header.typeID = API_WallID;
//		rElem.header.guid = inds[0];
//
//		err = ACAPI_Element_Get(&rElem);
//
//		if (err != NoError) return err;
//
//		BNZeroMemory(&attrib, sizeof(API_Attribute));
//		attrib.header.typeID = API_ProfileID;
//		attrib.header.index = rElem.wall.profileAttr;
//		err = ACAPI_Attribute_Get(&attrib);
//		//if (attrib.profile.wallType)
//		//{
//		//	Do_CreateTestProfileWall(i);
//		//}
//		if (err == NoError) {
//			err = ACAPI_Attribute_GetDefExt(API_ProfileID, attrib.header.index, &defs);
//			if (err == APIERR_BADID) {
//				BNZeroMemory(&defs, sizeof(API_AttributeDefExt));
//				err = NoError;
//			}
//			if (err == NoError) {
//				//WriteReport("\n\n****Profile name: %s", attrib.header.name);
//				ListProfileDescription(*defs.profile_vectorImageItems, *defs.profile_vectorImageParameterNames);
//			}
//			ACAPI_DisposeAttrDefsHdlsExt(&defs);
//		}
//
//		ACAPI_ELEMENT_MASK_CLEAR(rMask);
//		ACAPI_ELEMENT_MASK_SET(rMask, API_WallType, profileAttr);
//
//		err = ACAPI_CallUndoableCommand("Create text",
//			[&]() -> GSErrCode {
//			return ACAPI_Element_Change(&rElem, &rMask, nullptr, 0, true);
//		});
//	}
//
//	return err;
//}


/*
* Attribute helpers
*/
GSErrCode CAttributeHelpers::GetAttributeByName(const char *pAttrName, API_AttrTypeID eAttrType, API_Attribute &rDestAttr)
{
	BNZeroMemory(&rDestAttr, sizeof(rDestAttr));

	strcpy(rDestAttr.header.name, pAttrName);

	rDestAttr.header.typeID = eAttrType;
	GSErrCode nErr = ACAPI_Attribute_Search(&rDestAttr.header);

	if (nErr == NoError)
	{
		nErr = ACAPI_Attribute_Get(&rDestAttr);
	}

	return nErr;
}

const char* CAttributeHelpers::GetAttributeName(API_AttrTypeID eType, API_AttributeIndex nIndex)
{
	API_Attribute	attr;
	static char buffer[256] = { 0 };

	BNZeroMemory(buffer, sizeof(buffer));
	BNZeroMemory(&attr, sizeof(attr));
	attr.header.typeID = eType;
	attr.header.index = nIndex;
	if (ACAPI_Attribute_Get(&attr) == NoError) {
		CHTruncate(attr.header.name, buffer, sizeof(buffer));
	}
	else
		buffer[0] = ' ';

	return buffer;
}

GSErrCode CAttributeHelpers::GetAttributeByIndx(API_AttributeIndex nAttrIndx, API_AttrTypeID eAttrType, API_Attribute &rDestAttr)
{
	BNZeroMemory(&rDestAttr, sizeof(rDestAttr));

	rDestAttr.header.index = nAttrIndx;
	rDestAttr.header.typeID = eAttrType;

	GSErrCode nErr = ACAPI_Attribute_Get(&rDestAttr);

	return nErr;
}

GSErrCode CAttributeHelpers::DeleteAttributeByName(const char *pAttrName, API_AttrTypeID eAttrType)
{
	API_Attribute rAttrToDelete;

	GSErrCode nErr = GetAttributeByName(pAttrName, eAttrType, rAttrToDelete);

	if (nErr == NoError)
	{
		nErr = ACAPI_Attribute_Delete(&rAttrToDelete.header);
	}

	return nErr;
}

GSErrCode CAttributeHelpers::DeleteAttributeByIndx(API_AttributeIndex nAttrIndx, API_AttrTypeID eAttrType)
{
	API_Attribute rAttrToDelete;

	GSErrCode nErr = GetAttributeByIndx(nAttrIndx, eAttrType, rAttrToDelete);

	if (nErr == NoError)
	{
		nErr = ACAPI_Attribute_Delete(&rAttrToDelete.header);
	}

	return nErr;
}

// Globals
bool LogWall_Toggle()
{
	if (_prLogWallPalette) {
		LogWall_Destroy();
		return false;
	}

	_prLogWallPalette = new CCreateLogsDlg();
	if (_prLogWallPalette)
		_prLogWallPalette->Show();

	return true;
}


void LogWall_Destroy()
{
	if (_prLogWallPalette)
	{
		_prLogWallPalette->SendCloseRequest();
	}
}

void LogWall_OnDocChange()
{
	if (_prLogWallPalette)
		_prLogWallPalette->OnDocChanged();
}