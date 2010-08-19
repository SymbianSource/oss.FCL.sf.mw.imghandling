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

// main table
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

// temp table is identical to actual main table except it's not persistent
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

// version table
_LIT8( KThumbnailVersionTable, "CREATE TABLE ThumbnailVersion ("
        "Major INTEGER,"
        "Minor INTEGER,"
        "IMEI TEXT COLLATE NOCASE);");

// deleted thumbs table
_LIT8( KThumbnailDeletedTable, "CREATE TABLE ThumbnailDeleted ("
        // Assosiacted object in file system
        "Path TEXT UNIQUE COLLATE NOCASE);");

// indexes
_LIT8( KThumbnailCreateInfoTableIndex1, "CREATE INDEX idx1 ON ThumbnailInfo(Path, Size);");

// parameters
_LIT( KThumbnailSqlParamData, ":Data" );
_LIT( KThumbnailSqlParamFlags, ":Flags" );
_LIT( KThumbnailSqlParamPath, ":Path" );
_LIT( KThumbnailSqlParamNewPath, ":NewPath" );
_LIT( KThumbnailSqlParamWidth, ":Width" );
_LIT( KThumbnailSqlParamHeight, ":Height" );
_LIT( KThumbnailSqlParamOriginalWidth, ":OrigWidth" );
_LIT( KThumbnailSqlParamOriginalHeight, ":OrigHeight" );
_LIT( KThumbnailSqlParamFormat, ":Format" );
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
_LIT( KThumbnailSqlParamLimit, ":Limit" );

// transaction 
_LIT8( KThumbnailBeginTransaction, "BEGIN TRANSACTION;" );
_LIT8( KThumbnailCommitTransaction, "COMMIT;" );
_LIT8( KThumbnailRollbackTransaction, "ROLLBACK;" );

// version 
_LIT8( KThumbnailInsertToVersion, "INSERT INTO ThumbnailVersion (IMEI, Minor, Major) VALUES (:IMEI, :Minor,:Major);" );
_LIT8( KThumbnailSelectFromVersion, "SELECT * FROM ThumbnailVersion LIMIT 1" );

// IMEI
_LIT8( KThumbnailUpdateIMEI, "UPDATE ThumbnailVersion SET IMEI = :IMEI" );

// rowIDs
_LIT8 ( KGetInfoRowID, "SELECT MAX (ThumbnailInfo.rowID) FROM ThumbnailInfo" );
_LIT8 ( KGetDataRowID, "SELECT MAX (ThumbnailInfoData.rowID) FROM ThumbnailInfoData" );

// flush 
_LIT8( KThumbnailMoveFromTempInfoToMainTable, "INSERT INTO ThumbnailInfo SELECT * FROM TempThumbnailInfo;");
_LIT8( KThumbnailMoveFromTempDataToMainTable, "INSERT INTO ThumbnailInfoData SELECT * FROM TempThumbnailInfoData;");
_LIT8( KThumbnailDeleteFromTempInfoTable, "DELETE FROM TempThumbnailInfo;");
_LIT8( KThumbnailDeleteFromTempDataTable, "DELETE FROM TempThumbnailInfoData;");

// store thumb
_LIT8( KThumbnailInsertTempThumbnailInfo, "INSERT INTO TempThumbnailInfo "
        "(Path,Size,Format,Width,Height,OrigWidth,OrigHeight,Flags,Orientation,ThumbFromPath,Modified) "
        "VALUES "
        "(:Path,:Size,:Format,:Width,:Height,:OrigWidth,:OrigHeight,:Flags,:Orient,:ThumbFromPath,:Modified);" );

_LIT8( KThumbnailInsertTempThumbnailInfoData, "INSERT INTO TempThumbnailInfoData (Data) VALUES (:Data);" );

// duplicate check
_LIT8 ( KThumbnailTempFindDuplicate, "SELECT Path FROM TempThumbnailInfo WHERE Path = :Path AND Size = :Size;" );
_LIT8 ( KThumbnailFindDuplicate, "SELECT Path FROM ThumbnailInfo WHERE Path = :Path AND Size = :Size;" );

// select size
_LIT8( KThumbnailSelectSizeByPath, "SELECT Size FROM ThumbnailInfo "
        "WHERE NOT EXISTS (SELECT Path FROM ThumbnailDeleted "
        "WHERE ThumbnailInfo.Path = ThumbnailDeleted.Path) "
        "AND Path = :Path ORDER BY Size DESC;" );

_LIT8( KThumbnailSelectTempSizeByPath, "SELECT Size FROM TempThumbnailInfo WHERE Path = :Path ORDER BY Size DESC;" );

// select timestamp
_LIT8( KThumbnailSelectModifiedByPath, "SELECT Modified FROM ThumbnailInfo WHERE Path = :Path"  );
_LIT8( KThumbnailSelectTempModifiedByPath, "SELECT Modified FROM TempThumbnailInfo WHERE Path = :Path");

// select thumb
_LIT8( KThumbnailSelectInfoByPath, "SELECT ThumbnailInfo.Format, ThumbnailInfoData.Data, ThumbnailInfo.Width, ThumbnailInfo.Height, ThumbnailInfo.Flags "
        "FROM ThumbnailInfo "
        "JOIN ThumbnailInfoData "
        "ON ThumbnailInfo.RowID = ThumbnailInfoData.RowID "
        "WHERE NOT EXISTS (SELECT Path FROM ThumbnailDeleted "
        "WHERE ThumbnailInfo.Path = ThumbnailDeleted.Path) " 
        "AND ThumbnailInfo.Path = :Path AND ThumbnailInfo.Size = :Size;");

_LIT8( KThumbnailSelectTempInfoByPath, "SELECT TempThumbnailInfo.Format, TempThumbnailInfoData.Data, TempThumbnailInfo.Width, TempThumbnailInfo.Height, TempThumbnailInfo.Flags "
        "FROM TempThumbnailInfo "
        "JOIN TempThumbnailInfoData "
        "ON TempThumbnailInfo.RowID = TempThumbnailInfoData.RowID "
        "WHERE TempThumbnailInfo.Path = :Path AND TempThumbnailInfo.Size = :Size;");

// delete thumb
_LIT8( KThumbnailSqlSelectRowIDInfoByPath, "SELECT ThumbnailInfo.RowID FROM ThumbnailInfo WHERE Path = :Path;" );
_LIT8( KThumbnailSqlDeleteInfoByPath, "DELETE FROM ThumbnailInfo WHERE ThumbnailInfo.RowID = :RowID;" );
_LIT8( KThumbnailSqlDeleteInfoDataByPath, "DELETE FROM ThumbnailInfoData WHERE ThumbnailInfoData.RowID = :RowID;" );
_LIT8( KTempThumbnailSqlSelectRowIDInfoByPath, "SELECT TempThumbnailInfo.RowID FROM TempThumbnailInfo WHERE Path = :Path LIMIT 1;" );
_LIT8( KTempThumbnailSqlDeleteInfoByPath, "DELETE FROM TempThumbnailInfo WHERE TempThumbnailInfo.RowID = :RowID;" );
_LIT8( KTempThumbnailSqlDeleteInfoDataByPath, "DELETE FROM TempThumbnailInfoData WHERE TempThumbnailInfoData.RowID = :RowID;" );

// mark deleted
_LIT8( KThumbnailSqlInsertDeleted, "INSERT INTO ThumbnailDeleted (Path) VALUES (:Path);" );
_LIT8( KThumbnailSqlFindDeleted, "SELECT * FROM ThumbnailDeleted WHERE Path = :Path;" );

// delete marked
_LIT8( KThumbnailSqlSelectMarked, "SELECT ThumbnailInfo.RowID FROM ThumbnailInfo "
        "WHERE EXISTS (SELECT Path FROM ThumbnailDeleted "
        "WHERE ThumbnailInfo.Path = ThumbnailDeleted.Path) LIMIT :Limit;" );

_LIT8( KThumbnailSqlDeleteInfoByRowID, "DELETE FROM ThumbnailInfo WHERE ThumbnailInfo.RowID = :RowID;" );
_LIT8( KThumbnailSqlDeleteInfoDataByRowID, "DELETE FROM ThumbnailInfoData WHERE ThumbnailInfoData.RowID = :RowID;" );

_LIT8( KThumbnailSqlDeleteFromDeleted, "DELETE FROM ThumbnailDeleted "
        "WHERE NOT EXISTS (SELECT Path FROM ThumbnailInfo "
        "WHERE ThumbnailDeleted.Path = ThumbnailInfo.Path);" );

// rename thumb
_LIT8( KThumbnailRename, "UPDATE ThumbnailInfo SET Path = :NewPath WHERE Path = :Path;" );
_LIT8( KThumbnailTempRename, "UPDATE TempThumbnailInfo SET Path = :NewPath WHERE Path = :Path;" );

// reset blacklisted
_LIT8( KThumbnailTouchBlacklistedRows, "UPDATE ThumbnailInfo SET Flags = 4, Modified = 0 WHERE Flags & :Flag" );

// existence check
_LIT8( KThumbnailSelectAllPaths, "SELECT ThumbnailInfo.RowID,Path FROM ThumbnailInfo "
        "WHERE NOT EXISTS (SELECT Path FROM ThumbnailDeleted "
        "WHERE ThumbnailInfo.Path = ThumbnailDeleted.Path) "
        "AND ThumbFromPath = 1 AND ThumbnailInfo.RowID > :RowID ORDER BY ThumbnailInfo.RowID ASC LIMIT :Limit;" );

#endif // THUMBNAILSQL_H
