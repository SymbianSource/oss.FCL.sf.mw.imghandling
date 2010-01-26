/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  SQL statements
 *
*/


#ifndef THUMBNAILSQL_H
#define THUMBNAILSQL_H

//main table
_LIT8( KThumbnailCreateInfoTable, "CREATE TABLE ThumbnailInfo ("
        // Assosiacted object in file system
        "Path TEXT COLLATE NOCASE,"
        // Associated (MDS) Id 
        "TNId INTEGER,"
        // Combined (enumerated) size and classification 
        "Size INTEGER,"
        // Format of thumbnail (CFBsbitmap, JPeg...)
        "Format INTEGER,"
        // If thumbnail is in the filesystem then this is NOT NULL and points to such file
        "TNPath TEXT COLLATE NOCASE,"
        // Width of thumbnail
        "Width INTEGER,"
        // Height of thumbnail
        "Height INTEGER,"
        // Width of source
        "OrigWidth INTEGER,"
        // Height of source
        "OrigHeight INTEGER,"
        // Control flags, cropped etc.
        "Flags INTEGER,"
        // Frame number of video TN when user defined
        "VideoPosition INTEGER,"
        // Thumbnail orientation
        "Orientation INTEGER,"
        // Set if thumbnail is actually created from the associated object path
        "ThumbFromPath INTEGER,"
        // Last modified timestamp
        "Modified LARGEINT);");
        
_LIT8( KThumbnailCreateInfoDataTable, "CREATE TABLE ThumbnailInfoData ("
        // If Thumbnail is stored in SQL then Data is NOT NULL
        "Data BLOB);");
		
//Create index for speedup DB searches 
_LIT8( KThumbnailCreateInfoTableIndex1, "CREATE INDEX idx1 ON ThumbnailInfo(Path, Size);");
_LIT8( KThumbnailCreateInfoTableIndex2, "CREATE INDEX idx2 ON ThumbnailInfo(TNId, Size);");

//temp table is identical to actual main table except it's not persistent
_LIT8( KThumbnailCreateTempInfoTable, "CREATE TEMP TABLE TempThumbnailInfo ("
        "Path TEXT COLLATE NOCASE,"
        "TNId INTEGER,"
        "Size INTEGER,"
        "Format INTEGER,"
        "TNPath TEXT COLLATE NOCASE,"
        "Width INTEGER,"
        "Height INTEGER,"
        "OrigWidth INTEGER,"
        "OrigHeight INTEGER,"
        "Flags INTEGER,"
        "VideoPosition INTEGER,"
        "Orientation INTEGER,"
        "ThumbFromPath INTEGER,"
        "Modified LARGEINT);");

_LIT8( KThumbnailCreateTempInfoDataTable, "CREATE TEMP TABLE TempThumbnailInfoData ("
        // If Thumbnail is stored in SQL then Data is NOT NULL
        "Data BLOB);");

//version table
_LIT8( KThumbnailVersionTable, "CREATE TABLE ThumbnailVersion ("
        "Major INTEGER,"
        "Minor INTEGER,"
        "IMEI TEXT COLLATE NOCASE);");

_LIT8( KThumbnailMoveFromTempInfoToMainTable, "INSERT INTO ThumbnailInfo SELECT * FROM TempThumbnailInfo;");
_LIT8( KThumbnailMoveFromTempDataToMainTable, "INSERT INTO ThumbnailInfoData SELECT * FROM TempThumbnailInfoData;");

_LIT8( KThumbnailDeleteFromTempInfoTable, "DELETE FROM TempThumbnailInfo;");
_LIT8( KThumbnailDeleteFromTempDataTable, "DELETE FROM TempThumbnailInfoData;");

_LIT8( KThumbnailCreateSettingsTable, "CREATE TABLE ThumbnailSettings ("
    "Version INTEGER);" );

_LIT8( KThumbnailDropInfoTable, "DROP TABLE ThumbnailInfo;" );
_LIT8( KThumbnailDropTempInfoTable, "DROP TABLE TempThumbnailInfo;" );

_LIT8( KThumbnailDropSettingsTable, "DROP TABLE ThumbnailSettings;" );

_LIT8( KThumbnailBeginTransaction, "BEGIN TRANSACTION;" );
_LIT8( KThumbnailCommitTransaction, "COMMIT;" );
_LIT8( KThumbnailRollbackTransaction, "ROLLBACK;" );

_LIT8( KThumbnailInsertThumbnailInfoByPathAndId, "INSERT INTO TempThumbnailInfo "
    "(Path,TNId,Size,Format,Width,Height,OrigWidth,OrigHeight,Flags,Orientation,ThumbFromPath,Modified) ""VALUES "
    "(:Path,:TNId,:Size,:Format,:Width,:Height,:OrigWidth,:OrigHeight,:Flags,:Orient,:ThumbFromPath,:Modified);" );

_LIT8( KThumbnailInsertTempThumbnailInfoData, "INSERT INTO TempThumbnailInfoData (Data) VALUES (:Data);" );

_LIT8( KThumbnailSelectSizeByPath, "SELECT Size, TNId FROM ThumbnailInfo WHERE Path = :Path ORDER BY Size DESC;" );

_LIT8( KThumbnailSelectTempSizeByPath, "SELECT Size, TNId FROM TempThumbnailInfo WHERE Path = :Path ORDER BY Size DESC;" );

_LIT8( KThumbnailSelectById, "SELECT * "
        "FROM ThumbnailInfo "
        "JOIN ThumbnailInfoData "
        "ON ThumbnailInfo.RowID = ThumbnailInfoData.RowID "
        "WHERE TNId = :TNId" );

_LIT8( KThumbnailSelectTempById, "SELECT * "
        "FROM TempThumbnailInfo "
        "JOIN TempThumbnailInfoData "
        "ON TempThumbnailInfo.RowID = TempThumbnailInfoData.RowID "
        "WHERE TNId = :TNId" );

//query by Path
_LIT8( KThumbnailSelectInfoByPath, "SELECT ThumbnailInfo.Format, ThumbnailInfoData.Data, ThumbnailInfo.Width, ThumbnailInfo.Height, ThumbnailInfo.Flags "
        "FROM ThumbnailInfo "
        "JOIN ThumbnailInfoData "
        "ON ThumbnailInfo.RowID = ThumbnailInfoData.RowID "
        "WHERE ThumbnailInfo.Path = :Path AND ThumbnailInfo.Size = :Size;");

_LIT8( KThumbnailSelectTempInfoByPath, "SELECT TempThumbnailInfo.Format, TempThumbnailInfoData.Data, TempThumbnailInfo.Width, TempThumbnailInfo.Height, TempThumbnailInfo.Flags "
        "FROM TempThumbnailInfo "
        "JOIN TempThumbnailInfoData "
        "ON TempThumbnailInfo.RowID = TempThumbnailInfoData.RowID "
        "WHERE TempThumbnailInfo.Path = :Path AND TempThumbnailInfo.Size = :Size;");

//query by Id
_LIT8( KThumbnailSelectInfoById, "SELECT ThumbnailInfo.Format, ThumbnailInfoData.Data, ThumbnailInfo.Width, ThumbnailInfo.Height, ThumbnailInfo.Flags "
        "FROM ThumbnailInfo "
        "JOIN ThumbnailInfoData "
        "ON ThumbnailInfo.RowID = ThumbnailInfoData.RowID "
        "WHERE TNId = :TNId AND Size = :Size;" );

_LIT8( KThumbnailSelectTempInfoById, "SELECT TempThumbnailInfo.Format, TempThumbnailInfoData.Data, TempThumbnailInfo.Width, TempThumbnailInfo.Height, TempThumbnailInfo.Flags "
        "FROM TempThumbnailInfo "
        "JOIN TempThumbnailInfoData "
        "ON TempThumbnailInfo.RowID = TempThumbnailInfoData.RowID "
        "WHERE TNId = :TNId AND Size = :Size;" );
		
//query by Idv2
_LIT8( KThumbnailSelectInfoByIdv2, "SELECT ThumbnailInfo.Format, ThumbnailInfoData.Data, ThumbnailInfo.Width, ThumbnailInfo.Height, ThumbnailInfo.Flags "
        "FROM ThumbnailInfo "
        "JOIN ThumbnailInfoData "
        "ON ThumbnailInfo.RowID = ThumbnailInfoData.RowID "
        "WHERE TNId = :TNId AND (Size = :SizeImage OR Size = :SizeVideo OR Size = :SizeAudio);" );

_LIT8( KThumbnailSelectTempInfoByIdv2, "SELECT TempThumbnailInfo.Format, TempThumbnailInfoData.Data, TempThumbnailInfo.Width, TempThumbnailInfo.Height, TempThumbnailInfo.Flags "
        "FROM TempThumbnailInfo "
        "JOIN TempThumbnailInfoData "
        "ON TempThumbnailInfo.RowID = TempThumbnailInfoData.RowID "
        "WHERE TNId = :TNId AND (Size = :SizeImage OR Size = :SizeVideo OR Size = :SizeAudio);" );		

_LIT8( KThumbnailSelectSettings, "SELECT Version FROM ThumbnailSettings;" );

//qyery Path by ID
_LIT8( KThumbnailSelectPathByID, "SELECT Path FROM ThumbnailInfo WHERE TNId = :TNId;"  );
_LIT8( KThumbnailSelectTempPathByID, "SELECT Path FROM TempThumbnailInfo WHERE TNId = :TNId;");

//query Path and Modified by ID
_LIT8( KThumbnailSelectPathModifiedByID, "SELECT Path, Modified FROM ThumbnailInfo WHERE TNId = :TNId;"  );
_LIT8( KThumbnailSelectTempPathModifiedByID, "SELECT Path, Modified FROM TempThumbnailInfo WHERE TNId = :TNId;");

_LIT( KThumbnailSqlParamData, ":Data" );
_LIT( KThumbnailSqlParamFlags, ":Flags" );
_LIT( KThumbnailSqlParamPath, ":Path" );
_LIT( KThumbnailSqlParamWidth, ":Width" );
_LIT( KThumbnailSqlParamHeight, ":Height" );
_LIT( KThumbnailSqlParamOriginalWidth, ":OrigWidth" );
_LIT( KThumbnailSqlParamOriginalHeight, ":OrigHeight" );
_LIT( KThumbnailSqlParamFormat, ":Format" );
_LIT( KThumbnailSqlParamId, ":TNId" );
_LIT( KThumbnailSqlParamSize, ":Size" );
_LIT( KThumbnailSqlParamTNPath, ":TNPath" );
_LIT( KThumbnailSqlParamMajor, ":Major" );
_LIT( KThumbnailSqlParamMinor, ":Minor" );
_LIT( KThumbnailSqlParamImei, ":IMEI" );
_LIT( KThumbnailSqlParamSizeImage, ":SizeImage" );
_LIT( KThumbnailSqlParamSizeVideo, ":SizeVideo" );
_LIT( KThumbnailSqlParamSizeAudio, ":SizeAudio" );
_LIT( KThumbnailSqlParamRowID, ":RowID" );
_LIT( KThumbnailSqlParamOrientation, ":Orient" );
_LIT( KThumbnailSqlParamThumbFromPath, ":ThumbFromPath" );
_LIT( KThumbnailSqlParamModified, ":Modified" );
_LIT( KThumbnailSqlParamFlag, ":Flag" );

//Delete by path
_LIT8( KThumbnailSqlSelectRowIDInfoByPath, "SELECT ThumbnailInfo.RowID FROM ThumbnailInfo WHERE Path = :Path;" );
_LIT8( KThumbnailSqlDeleteInfoByPath, "DELETE FROM ThumbnailInfo WHERE ThumbnailInfo.RowID = :RowID;" );
_LIT8( KThumbnailSqlDeleteInfoDataByPath, "DELETE FROM ThumbnailInfoData WHERE ThumbnailInfoData.RowID = :RowID;" );
_LIT8( KTempThumbnailSqlSelectRowIDInfoByPath, "SELECT TempThumbnailInfo.RowID FROM TempThumbnailInfo WHERE Path = :Path LIMIT 1;" );
_LIT8( KTempThumbnailSqlDeleteInfoByPath, "DELETE FROM TempThumbnailInfo WHERE TempThumbnailInfo.RowID = :RowID;" );
_LIT8( KTempThumbnailSqlDeleteInfoDataByPath, "DELETE FROM TempThumbnailInfoData WHERE TempThumbnailInfoData.RowID = :RowID;" );


//Delete by ID
_LIT8( KThumbnailSqlSelectRowIDInfoByID, "SELECT ThumbnailInfo.RowID FROM ThumbnailInfo WHERE TNId = :TNId;" );
_LIT8( KThumbnailSqlDeleteInfoByID, "DELETE FROM ThumbnailInfo WHERE ThumbnailInfo.RowID = :RowID;" );
_LIT8( KThumbnailSqlDeleteInfoDataByID, "DELETE FROM ThumbnailInfoData WHERE ThumbnailInfoData.RowID = :RowID;" );
_LIT8( KTempThumbnailSqlSelectRowIDInfoByID, "SELECT TempThumbnailInfo.RowID FROM TempThumbnailInfo WHERE TNId = :TNId LIMIT 1;" );
_LIT8( KTempThumbnailSqlDeleteInfoByID, "DELETE FROM TempThumbnailInfo WHERE TempThumbnailInfo.RowID = :RowID;" );
_LIT8( KTempThumbnailSqlDeleteInfoDataByID, "DELETE FROM TempThumbnailInfoData WHERE TempThumbnailInfoData.RowID = :RowID;" );


//Update path by Id
_LIT8( KTempThumbnailSqlUpdateById, "UPDATE TempThumbnailInfo SET Path = :Path WHERE TNId = :TNId" );
_LIT8( KThumbnailSqlUpdateById, "UPDATE ThumbnailInfo SET Path = :Path WHERE TNId = :TNId" );

//version commands
_LIT8( KThumbnailInsertToVersion, "INSERT INTO ThumbnailVersion (IMEI, Minor, Major) VALUES (:IMEI, :Minor,:Major);" );
_LIT8( KThumbnailSelectFromVersion, "SELECT * FROM ThumbnailVersion LIMIT 1" );

//reset IDs
_LIT8( KTempThumbnailResetIDs, "UPDATE TempThumbnailInfo SET TNId = NULL WHERE TNId NOT NULL" );
_LIT8( KThumbnailResetIDs, "UPDATE ThumbnailInfo SET TNId = NULL WHERE TNId NOT NULL" );

//update IMEI
_LIT8( KThumbnailUpdateIMEI, "UPDATE ThumbnailVersion SET IMEI = :IMEI" );

//update ID by Path
_LIT8( KTempThumbnailUpdateIdByPath, "UPDATE TempThumbnailInfo SET TNId = :TNId WHERE Path = :Path" );
_LIT8( KThumbnailUpdateIdByPath, "UPDATE ThumbnailInfo SET TNId = :TNId WHERE Path = :Path" );

//qyery Modification timestamp by ID
_LIT8( KThumbnailSelectModifiedByID, "SELECT Modified FROM ThumbnailInfo WHERE TNId = :TNId AND ThumbFromPath = 1 LIMIT 1"  );
_LIT8( KThumbnailSelectTempModifiedByID, "SELECT Modified FROM TempThumbnailInfo WHERE TNId = :TNId AND ThumbFromPath = 1 LIMIT 1");

//query Modification timestamp by path
_LIT8( KThumbnailSelectModifiedByPath, "SELECT Modified FROM ThumbnailInfo WHERE Path = :Path"  );
_LIT8( KThumbnailSelectTempModifiedByPath, "SELECT Modified FROM TempThumbnailInfo WHERE Path = :Path");

// query possible duplicates
_LIT8 ( KTempFindDuplicate, "SELECT Path FROM TempThumbnailInfo WHERE Size = :Size AND (TNId = :TNId OR Path = :Path);" );
_LIT8 ( KFindDuplicate, "SELECT Path FROM ThumbnailInfo WHERE Size = :Size AND (TNId = :TNId OR Path = :Path);" );

// check rowIDs
_LIT8 ( KGetInfoRowID, "SELECT MAX (ThumbnailInfo.rowID) FROM ThumbnailInfo" );
_LIT8 ( KGetDataRowID, "SELECT MAX (ThumbnailInfoData.rowID) FROM ThumbnailInfoData" );

//remove KThumbnailDbFlagBlacklisted flag
_LIT8( KThumbnailRemoveBlacklistedFlag, "UPDATE ThumbnailInfo SET Flags = Flags & ~:Flag WHERE Flags & :Flag" );

#endif // THUMBNAILSQL_H
