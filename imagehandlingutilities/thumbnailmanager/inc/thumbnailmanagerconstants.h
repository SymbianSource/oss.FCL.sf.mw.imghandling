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
* Description:  Thumbnail manager constants
 *
*/


#ifndef THUMBNAILMANAGERCONSTANTS_H
#define THUMBNAILMANAGERCONSTANTS_H

#include <e32base.h>
#include <gdi.h>
#include <etel3rdparty.h>

#include <apmstd.h>

#include "thumbnailmanager.h" // TThumbnailFlags

class CThumbnailServerSession;

// P&S stuff
static _LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);
static _LIT_SECURITY_POLICY_C1(KPowerMgmtPolicy,ECapabilityPowerMgmt);

const TUid KTMPSNotification = { 0x102830AB };
const TUid KTAGDPSNotification = { 0x2001FD51 };
const TUid KMdSPSShutdown = { 0x20022E94 };
const TUid KServerIdle = { 0x102830AB };
const TUid KDaemonUid = { 0x2001FD51 };

const TInt KShutdown = 0x00000001;
const TInt KMdSShutdown = 0x00000002;
//used to signal from server side when it's idle
const TInt KIdle = 0x00000004;

//insert to temp table first wo indexing and move data to main table as batch
const TUint KMaxBatchItems = 18;

const TUint KMaxQueryItems = 1;

// maximum number of active client queue requests
const TUint KMaxClientRequests = 2;

// maximum number of active daemon requests
const TUint KMaxDaemonRequests = 3;

const TUint KClientRequestTimeout = 60000000; //60 sec
const TUint KClientRequestStartErrorTimeout = 100000; //100 ms

const TUint KThumbnailServerMajorVersionNumber = 0;
const TUint KThumbnailServerMinorVersionNumber = 1;
const TUint KThumbnailServerBuildVersionNumber = 1;

const TInt KThumbnailErrThumbnailNotFound = -62000;

//give MDS 1000 msec time to settle before starting generating TNs
const TInt KHarvestingCompleteTimeout = 10000000; //10 sec

const TInt KPSKeyTimeout = 10000000; //10 sec
//Store's auto flush timeout
const TInt KAutoFlushTimeout = 30000000; //30 sec

// minimum background generation idle time seconds
const TInt KBackgroundGenerationIdle = 2;

// video decoder timeout
const TInt KVideoDecoderTimeout = 5000000; // 5 seconds

const TDisplayMode KThumbnailDefaultDisplayMode = EColor64K;

//default displaymode (bpp - bits per pixel) for TNs in DB
//this makes possible to provide all colour depths up to 16M aka 24 -bit full colour
const TDisplayMode KStoreDisplayMode = EColor16M;

//required amount of memory to keep bitmaps on RAM in bits
const TInt KMemoryNeed = 5000000;

const TInt64 KDiskFullThreshold = 1024*1024*1; // 1 MB

_LIT( KThumbnailServerName, "ThumbnailServer" );
_LIT( KThumbnailServerProcess, "*ThumbnailServer*" );
_LIT( KThumbnailServerExe, "thumbnailserver.exe" );
_LIT( KThumbnailServerShutdown, "Thumbnailserver_Shutdown" );

_LIT( KTAGDaemonName, "ThumbAGDaemon" );
_LIT( KTAGDaemonProcess, "*ThumbAGDaemon*" );
_LIT( KTAGDaemonExe, "thumbagdaemon.exe" );

// server message slots, -1 doesn't reserve fixed amount from global but  uses free available amount instead
const TInt KMessageSlots = -1;

const TInt KMinPriority = KMinTInt;
const TInt KMinPlaceholderPriority = (KMinTInt +1000);
const TInt KLowPriority = -1000;
const TInt KNormalPriority = 0;
const TInt KHighPriority = 1000;
const TInt KMaxGeneratePriority = (KMaxTInt -1000);
const TInt KMaxPriority = KMaxTInt; // For scaling tasks
const TInt KImeiBufferSize = CTelephony::KPhoneSerialNumberSize;
const TInt KCheckValue = 123456;

_LIT8( KJpegMime,    "image/jpeg" );            _LIT( KJpegExt, ".jpeg" );
_LIT8( KJpeg2000Mime,    "image/jp2" );            _LIT( KJpeg2000Ext, ".jp2" );
_LIT8( KJpgMime,    "image/jpeg" );            _LIT( KJpgExt, ".jpg" );
_LIT8( KGifMime,     "image/gif" );             _LIT( KGifExt,  ".gif" );
_LIT8( KPngMime,     "image/png" );             _LIT( KPngExt,  ".png" ); 
_LIT8( KSvgMime,     "image/svg+xml" );             _LIT( KSvgExt,  ".svg" ); 
_LIT8( KMpgMime1,    "video/mpeg");             _LIT( KMpgExt1,  ".mpg" );
_LIT8( KMpeg4Mime,   "video/mpeg4" );           _LIT( KMpeg4Ext,".mpeg4" );
_LIT8( KMp4Mime,     "video/mp4" );             _LIT( KMp4Ext,  ".mp4" );
_LIT8( KAviMime,    "video/x-msvideo" );       _LIT( KAviExt,  ".avi" );
_LIT8( KMp3Mime,    "audio/mpeg" );           _LIT( KMp3Ext,  ".mp3" );
_LIT8( KNonEmbeddArtMime,    "audio/mpeg" );           _LIT( KNonEmbeddArtExt,  ".alb" );
_LIT8( KM4aMime,    "audio/mp4" );           _LIT( KM4aExt,  ".m4a" );
_LIT8( KAacMime,     "audio/aac" );             _LIT( KAacExt,  ".aac" );
_LIT8( KWmaMime,     "audio/x-ms-wma" );        _LIT( KWmaExt,  ".wma" );
_LIT8( KBmpMime,     "image/bmp" );             _LIT( KBmpExt,  ".bmp" );
_LIT8( KAudio3gppMime,     "audio/3gpp" ); 
_LIT8( KVideo3gppMime,     "video/3gpp" );  _LIT( K3gpExt,  ".3gp" );
_LIT8( KAudioAmrMime,     "audio/AMR" );     _LIT( KAmrExt,  ".amr" );
_LIT8( KVideoWmvMime, "video/x-ms-wmv" );     _LIT( KWmvExt,    ".wmv" );
_LIT8( KRealAudioMime, "audio/vnd.rn-realaudio" );        _LIT( KRealAudioExt,    ".ra" );
_LIT8( KPmRealAudioPluginMime, "audio/x-pn-realaudio-plugin" ); _LIT( KPmRealAudioPluginExt,    ".rpm" );
_LIT8( KPmRealVideoPluginMime, "video/x-pn-realvideo" ); _LIT( KPmRealVideoPluginExt,    ".rm" );
_LIT8( KPmRealVbVideoPluginMime, "video/x-pn-realvideo" ); _LIT( KPmRealVbVideoPluginExt,    ".rmvb" );
_LIT8( KPmRealAudioMime, "audio/x-pn-realaudio" );        _LIT( KPmRealAudioExt,    ".ra" );
_LIT8( KRealVideoMime, "video/vnd.rn-realvideo" );        _LIT( KRealVideoExt,    ".rv" );
_LIT8( KFlashVideoMime,    "video/x-flv" );       _LIT( KFlashVideoExt,  ".flv" );
_LIT8( KMatroskaVideoMime,    "video/x-matroska" );       _LIT( KMatroskaVideoExt,  ".mkv" );
_LIT( KImageMime, "image/*" );
_LIT( KVideoMime, "video/*" );
_LIT( KAudioMime, "audio/*" );
_LIT( KM4vExt,  ".m4v" );
_LIT( KNonEmbeddedArtExt, ".alb" );

/**
 *  Control flags set by the server for handling specific situations
 *  (for example for distinguishing between preview thumbnails and
 *  final thumbnails).
 *
 *  @since S60 v5.0
 */
enum TThumbnailControlFlags
    {
    /**
     * Default value. No flags set.
     */
    EThumbnailNoControlFlags = 0, 

    /**
     * Set by the server when the request is completed and it is only the
     * first part of a two-phase request
     */
    EThumbnailPreviewThumbnail = 1,
    
    /**
     * Set by the client to inform server to create only missing persistent sizes thumbnails
     */
    EThumbnailGeneratePersistentSizesOnly = 2 
};


/**
 *  Thumbnail request parameters used for client-server communication.
 *
 *  @since S60 v5.0
 */
struct TThumbnailRequestParams
    {
public:
    /**
     * Bitmap handle for completed requests
     */
    TInt iBitmapHandle;

    /**
     * Flags for new requests.
     */
    CThumbnailManager::TThumbnailFlags iFlags;

    /**
     * Quality-preference value for new requests.
     */
    CThumbnailManager::TThumbnailQualityPreference iQualityPreference;

    /**
     * Priority for new requests.
     */
    TInt iPriority;

    /**
     * Requested thumbnail size new requests.
     */
    TSize iSize;

    /**
     * Requested display mode new requests.
     */
    TDisplayMode iDisplayMode;

    /**
     * Full path to object file for new requests. Should be set even
     * when file handles are used.
     */
    TFileName iFileName;

    /**
     * Full path to object to which the imported thumb is to be linked.
     */
    TFileName iTargetUri;
 
    /**
     * Thumbnail ID
     */   
    TThumbnailId iThumbnailId;
    
    /**
     * Relative thumbnail size
     */       
    TThumbnailSize iThumbnailSize;
    
    /**
     * MIME type
     */       
    TDataType iMimeType;

    /**
     * Image buffer used to create & set the thumbnail
     */       
    TDesC8* iBuffer;
    
    /**
     * Session specific request ID allocated by the client.
     */
    TThumbnailRequestId iRequestId;

    /**
     * Control flags set by the server for handling specific situations
     * (for example for distinguishing between preview thumbnails and
     * final thumbnails).
     */
    TThumbnailControlFlags iControlFlags;
    
    /**
     * Thumbnail's modify timestamp
     */
    TInt64 iModified;
    
    /**
     * Thumbnail's orientation
     */
    TInt iOrientation;
    };


typedef TPckg < TThumbnailRequestParams > TThumbnailRequestParamsPckg;
typedef TPckgBuf < TThumbnailRequestParams > TThumbnailRequestParamsPckgBuf;


/**
 *  Request ID class used on the server side. Consists of a pointer to a
 *  session and a session specific ID.
 *
 *  @since S60 v5.0
 */
struct TThumbnailServerRequestId
    {
    /**
     * Default C++ constructor
     *
     * @since S60 v5.0
     */
    inline TThumbnailServerRequestId(): iSession( NULL ), iRequestId( 0 ){}

    /**
     * C++ constructor
     *
     * @since S60 v5.0
     * @param aSession Pointer to the server-side session object, which
     *                 created the request.
     * @param aRequestId Session specific request ID as allocated by the
     *                   client.
     */
    inline TThumbnailServerRequestId( CThumbnailServerSession* aSession,
        TThumbnailRequestId aRequestId ): iSession( aSession ), iRequestId(
        aRequestId ){}

    /**
     * Compare request IDs. Both session and client-side request ID must
     * match.
     *
     * @param aRequestId Another TThumbnailServerRequestId to compare to
     * @since S60 v5.0
     */
    inline TBool operator == ( const TThumbnailServerRequestId& aRequestId )
        const
        {
        return aRequestId.iSession == iSession && aRequestId.iRequestId ==
            iRequestId;
    } 

public:
    /**
     * Pointer to the server-side session object, which created the request.
     * Not own.
     */
    CThumbnailServerSession* iSession;

    /**
     * Session specific request ID as allocated by the client.
     */
    TThumbnailRequestId iRequestId;
};

/**
 *  Client-server message IDs
 *
 *  @since S60 v5.0
 *  Start from 0 so that TPolicy range matches to function count.
 */
enum TThumbnailServerRequest
    {
    /**
     * Thumbnail request using file path. A TThumbnailRequestParams
     * struct is passed as a parameter.
     * @see TThumbnailRequestParams
     */
    ERequestThumbByPathAsync = 0, 

    /**
     * Thumbnail request using file path. A TThumbnailRequestParams
     * struct is passed as a parameter as well as the file handle using
     * TransferToServer()/AdoptFromClient().
     * @see TThumbnailRequestParams
     */
    ERequestThumbByFileHandleAsync,

    /**
     * Release a bitmap after the client callback has returned. Bitmap
     * handle is passed as a parameter.
     */
    EReleaseBitmap, 

    /**
     * Cancel a thumbnail request. Session specific request ID is passed
     * as a parameter.
     */
    ECancelRequest, 

    /**
     * Change the priority of a thumbnail request. Session specific request
     * ID and new priority value are passed as parameters.
     */
    EChangePriority, 

    /**
     * Create thumbnails for a file. File path is passed as a
     * parameter.
     */
    ECreateThumbnails, 

    /**
     * Delete existing thumbnails for a file. File path is passed as a
     * parameter.
     */
    EDeleteThumbnails, 

    /**
     * Get the required size (in characters) for a buffer that contains the
     * list of supported MIME types. Size in integers is returned in the
     * first parameter.
     */
    EGetMimeTypeBufferSize, 

    /**
     * Get the list of supported MIME types and store them as the first
     * parameter. The first parameter should be allocated by the client
     * to be large enough (using EGetMimeTypeBufferSize).
     */
    EGetMimeTypeList,
	   
    /**
     * Thumbnail request using ID. 
     */    
    ERequestThumbByIdAsync,
    
    ERequestThumbByBufferAsync,

    /**
     * Request to set thumbnail created from buffered image
     */        
    ERequestSetThumbnailByBuffer,
    
    /**
     * Delete existing thumbnails. Id as parameter.
     */
    EDeleteThumbnailsById,
    
    EReserved1,
    
    /**
     * Update thumbnails by given Id.
    */
    EUpdateThumbnails,
    
    /**
     * Request to set thumbnail created from given bitmap
     */        
    ERequestSetThumbnailByBitmap,
 
    /**
     * Do not remove and keep as last item! Holds count of functions supported.
     */        
    EThumbnailServerRequestCount
   };

/**
 *  Thumbnail format in storage
 *
 *  @since S60 v5.0
 */
enum TThumbnailFormat
    {
	/**
	* Symbian internal bitmap format, usually as extranlized BLOB in the db.
	*/
    EThumbnailFormatFbsBitmap,
	/**
	* Stantard JPEG
	*/
    EThumbnailFormatJpeg
    };

struct TThumbnailDatabaseData
    {
public:
    /**
    * Full path to object to which the imported thumb is to be linked.
    */
    TPath iPath;
    /**
    * Thumbnail ID
    */      
    TInt iTnId;
    /**
    * Requested thumbnail size new requests.
    */
    TInt iSize;
    /**
    * type of data
    */ 
    TInt iFormat;
    /**
    * Path for the thumbnails
    */  
    TPath iTnPath;
    /**
    * Data if bitmap
    */  
    CFbsBitmap* iBlob;
    /**
    * Data if jpeg
    */
    TDesC8* iData;
   /**
    * Width of thumbnail
    */  
    TInt iWidth;
    /**
    * Height of thumbnail
    */  
    TInt iHeight;
    /**
    * Original width of thumbnail
    */  
    TInt iOrigWidth;
    /**
    * Original height of thumbnail
    */  
    TInt iOrigHeight;
    /**
    * flags
    */  
    TInt iFlags;
    /**
    * videoposition
    */
    TInt iVideoPosition;
    /**
    * thumb oritentation
    */  
    TInt iOrientation;
    /**
    * Thumb created from associated path
    */  
    TInt iThumbFromPath;
    /**
    * last modified
    */
    TInt64 iModified;
    
    };

/**
 *  MDS query modes used during thumbnail generation
 *
 *  @since S60 v5.0
 */
enum TMDSQueryType
    {
    /**
     * Query Id by Path
     */
    EId,
    /**
     * Query Path by Id
     */
    EURI
    };

#endif // THUMBNAILMANAGERCONSTANTS_H
