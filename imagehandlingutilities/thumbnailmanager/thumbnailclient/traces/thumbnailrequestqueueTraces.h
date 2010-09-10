// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILREQUESTQUEUETRACES_H__
#define __THUMBNAILREQUESTQUEUETRACES_H__

#define KOstTraceComponentID 0x102830aa

#define CTHUMBNAILREQUESTQUEUE_CTHUMBNAILREQUESTQUEUE 0x86004b
#define DUP1_CTHUMBNAILREQUESTQUEUE_CTHUMBNAILREQUESTQUEUE 0x86004c
#define CTHUMBNAILREQUESTQUEUE_PROCESS 0x86004d
#define DUP4_CTHUMBNAILREQUESTQUEUE_PROCESS 0x86004e
#define DUP5_CTHUMBNAILREQUESTQUEUE_PROCESS 0x86004f
#define DUP1_CTHUMBNAILREQUESTQUEUE_PROCESS 0x860050
#define DUP2_CTHUMBNAILREQUESTQUEUE_PROCESS 0x860051
#define DUP3_CTHUMBNAILREQUESTQUEUE_PROCESS 0x860052
#define CTHUMBNAILREQUESTQUEUE_ADDREQUESTL 0x860053
#define DUP1_CTHUMBNAILREQUESTQUEUE_REMOVECOMPLETED 0x860054
#define DUP2_CTHUMBNAILREQUESTQUEUE_REMOVECOMPLETED 0x860055
#define DUP3_CTHUMBNAILREQUESTQUEUE_REMOVECOMPLETED 0x860056
#define CTHUMBNAILREQUESTQUEUE_CANCELREQUEST 0x860057
#define DUP1_CTHUMBNAILREQUESTQUEUE_CANCELREQUEST 0x860058
#define DUP2_CTHUMBNAILREQUESTQUEUE_CANCELREQUEST 0x860059
#define CTHUMBNAILREQUESTQUEUE_CHANGEPRIORITY 0x86005a
#define CTHUMBNAILREQUESTQUEUE_REQUESTCOMPLETE 0x86005b


#ifndef __OSTTRACEGEN2_TUINT32_TINT_TINT__
#define __OSTTRACEGEN2_TUINT32_TINT_TINT__

inline TBool OstTraceGen2( TUint32 aTraceID, TInt aParam1, TInt aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TINT_TINT__


#ifndef __OSTTRACEGEN2_TUINT32_TINT32_TINT32__
#define __OSTTRACEGEN2_TUINT32_TINT32_TINT32__

inline TBool OstTraceGen2( TUint32 aTraceID, TInt32 aParam1, TInt32 aParam2 )
    {
    TBool retval = BTraceFiltered8( EXTRACT_GROUP_ID(aTraceID), EOstTraceActivationQuery, KOstTraceComponentID, aTraceID );
    if ( retval )
        {
        TUint8 data[ 8 ];
        TUint8* ptr = data;
        *( ( TInt* )ptr ) = aParam1;
        ptr += sizeof ( TInt );
        *( ( TInt* )ptr ) = aParam2;
        ptr += sizeof ( TInt );
        ptr -= 8;
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, 8 );
        }
    return retval;
    }

#endif // __OSTTRACEGEN2_TUINT32_TINT32_TINT32__



#endif

// End of file

