// Created by TraceCompiler 2.2.3
// DO NOT EDIT, CHANGES WILL BE LOST

#ifndef __THUMBNAILMDSQUERYTASKTRACES_H__
#define __THUMBNAILMDSQUERYTASKTRACES_H__

#define KOstTraceComponentID 0x102830ab

#define CTHUMBNAILMDSQUERYTASK_CTHUMBNAILMDSQUERYTASK 0x860039
#define DUP1_CTHUMBNAILMDSQUERYTASK_CTHUMBNAILMDSQUERYTASK 0x86003a
#define CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED 0x86003b
#define DUP1_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED 0x86003c
#define DUP2_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED 0x86003d
#define DUP3_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED 0x86003e
#define DUP4_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED 0x86003f
#define DUP5_CTHUMBNAILMDSQUERYTASK_HANDLEQUERYCOMPLETED 0x860040
#define CTHUMBNAILMDSQUERYTASK_STARTL 0x860041
#define CTHUMBNAILMDSQUERYTASK_RUNL 0x860042
#define CTHUMBNAILMDSQUERYTASK_DOCANCEL 0x860043
#define CTHUMBNAILMDSQUERYTASK_QUERYPATHBYIDL 0x860044
#define CTHUMBNAILMDSQUERYTASK_SETUPDATETODB 0x860045


#ifndef __KERNEL_MODE__
#ifndef __OSTTRACEGEN1_TUINT32_CONST_TDESC16REF__
#define __OSTTRACEGEN1_TUINT32_CONST_TDESC16REF__

inline TBool OstTraceGen1( TUint32 aTraceID, const TDesC16& aParam1 )
    {
    TBool retval;
    TInt size = aParam1.Size();
    // BTrace assumes that parameter size is atleast 4 bytes
    if (size % 4 == 0)
        {
        TUint8* ptr = ( TUint8* )aParam1.Ptr();
        // Data is written directly and length is determined from trace message length
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, size );
        }
    else
        {
        TUint8 data[ KOstMaxDataLength ];
        TUint8* ptr = data;
        if (size > KOstMaxDataLength)
            {
            size = KOstMaxDataLength;
            }
        TInt sizeAligned = ( size + 3 ) & ~3;
        memcpy( ptr, aParam1.Ptr(), size );
        ptr += size;
        // Fillers are written to get 32-bit alignment
        while ( size++ < sizeAligned )
            {
            *ptr++ = 0;
            }
        ptr -= sizeAligned;
        size = sizeAligned;
        // Data is written directly and length is determined from trace message length
        retval = OstSendNBytes( EXTRACT_GROUP_ID(aTraceID), EOstTrace, KOstTraceComponentID, aTraceID, ptr, size );
        }
    return retval;
    }

#endif // __OSTTRACEGEN1_TUINT32_CONST_TDESC16REF__

#endif


#endif

// End of file

