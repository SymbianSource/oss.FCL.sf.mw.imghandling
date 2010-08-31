/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description: 
*
*/

#include <fbs.h>
#include <QtCore>
#include <qpixmap.h>
#include <thumbnailmanager.h>
#include <thumbnaildata.h>

#include "thumbnailmanager_qt.h"
#include "thumbnailmanager_p_qt.h"

#include <e32base.h>

const int ThumbnailMangerPriorityLowest = CActive::EPriorityIdle;
const int ThumbnailMangerPriorityHighest = CActive::EPriorityHigh;

ThumbnailManagerPrivate::ThumbnailManagerPrivate() : 
    connectionCounterImage( 0 ),
    connectionCounterPixmap( 0 ),
    iThumbnailManager( NULL ), 
    byteArray( NULL )
{
    TRAP_IGNORE(
        iThumbnailManager = CThumbnailManager::NewL( *this );
        iThumbnailManager->SetDisplayModeL( EColor64K );
        );
}

ThumbnailManagerPrivate::~ThumbnailManagerPrivate()
{
    if( iThumbnailManager ){
        delete iThumbnailManager;
    }
    if( byteArray ){
        delete byteArray;
    }
}

ThumbnailManager::QualityPreference ThumbnailManagerPrivate::qualityPreference() const
{
    return static_cast<ThumbnailManager::QualityPreference>( iThumbnailManager->QualityPreference() );
}

bool ThumbnailManagerPrivate::setQualityPreference( ThumbnailManager::QualityPreference qualityPreference )
{
    TRAPD( err, iThumbnailManager->SetQualityPreferenceL( static_cast<CThumbnailManager::TThumbnailQualityPreference>(qualityPreference) ));
    return ( err == KErrNone );
}

QSize ThumbnailManagerPrivate::thumbnailSize() const
{
    TSize tsize( iThumbnailManager->ThumbnailSize() );
    QSize qsize( tsize.iWidth, tsize.iHeight);
    return qsize;
}

bool ThumbnailManagerPrivate::setThumbnailSize( const QSize& thumbnailSize )
{
    TRAPD( err, iThumbnailManager->SetThumbnailSizeL( TSize( thumbnailSize.width(), thumbnailSize.height() ) ) );
    return ( err == KErrNone );
}

bool ThumbnailManagerPrivate::setThumbnailSize( ThumbnailManager::ThumbnailSize thumbnailSize )
{
    TThumbnailSize size(EUnknownThumbnailSize);
    switch( thumbnailSize ){
    case ThumbnailManager::ThumbnailSmall:
        size = EListThumbnailSize;
        break;
    case ThumbnailManager::ThumbnailMedium:
        size = EGridThumbnailSize;
        break;
    case ThumbnailManager::ThumbnailLarge:
        size = EFullScreenThumbnailSize;
        break;
    default:
        break;
    }

    TRAPD( err, iThumbnailManager->SetThumbnailSizeL( size ) );
    return ( err == KErrNone );
}

ThumbnailManager::ThumbnailMode ThumbnailManagerPrivate::mode() const
{
    return static_cast<ThumbnailManager::ThumbnailMode>( iThumbnailManager->Flags() );
}

bool ThumbnailManagerPrivate::setMode( ThumbnailManager::ThumbnailMode mode )
{
    TRAPD( err, iThumbnailManager->SetFlagsL( static_cast<CThumbnailManager::TThumbnailFlags>( mode ) ) );
    return ( err == KErrNone );
}

int ThumbnailManagerPrivate::getThumbnail( const QString& fileName, void* clientData, 
        int priority )
{
    int result( -1 );
    QString symbFileName( fileName );
    
    priority = convertPriority(priority);

    if( symbFileName.contains( "/" ) )
        symbFileName.replace( "/", "\\", Qt::CaseSensitive );
    
    RBuf buf;
    TRAP_IGNORE( 
        CleanupClosePushL( buf );
        buf.CreateL( symbFileName.length() );
        buf.Copy( symbFileName.utf16(), symbFileName.length() );
        
        CThumbnailObjectSource* objSrc = CThumbnailObjectSource::NewLC( buf, KNullDesC );
        result = iThumbnailManager->GetThumbnailL( *objSrc, clientData, priority );
        CleanupStack::PopAndDestroy( 2, &buf );
    );
    return result;
}

int ThumbnailManagerPrivate::getThumbnail( unsigned long int aThumbnailId, void* clientData, 
        int priority )
{
    int result( -1 );
    
    priority = convertPriority(priority);
    
    TRAP_IGNORE( result = iThumbnailManager->GetThumbnailL( aThumbnailId, clientData, priority ));
    return result;
}    

int ThumbnailManagerPrivate::setThumbnail( const QPixmap& source, const QString& fileName, 
        void * clientData, int priority )
{
    int result( -1 );
    RBuf file;
    _LIT( mime, "image/png" );
    
    priority = convertPriority(priority);

    if( !byteArray ){
        byteArray = new QByteArray();
    }
    QBuffer buffer( byteArray );
    buffer.open( QBuffer::ReadWrite );
    QDataStream dataStream( &buffer );

    dataStream << source;

    int offset = ( dataStream.version() >= 5 ? 4 : 0 );
    
    TRAP_IGNORE( 
        CleanupClosePushL( file );
        file.CreateL( fileName.length() );
        file.Copy( fileName.utf16(), fileName.length() );

        HBufC* mimetype = HBufC::NewLC( 9 );
        mimetype->Des() = mime();

        TPtrC8* ptr = new TPtrC8( reinterpret_cast<const TUint8*>( byteArray->data() + offset ), byteArray->count() - offset );
                
        CThumbnailObjectSource* objSrc = CThumbnailObjectSource::NewLC( ptr, *mimetype, file );
        result = iThumbnailManager->SetThumbnailL( *objSrc, clientData, priority );
        CleanupStack::PopAndDestroy( 3, &file );
    );
    return result;
}

int ThumbnailManagerPrivate::setThumbnail( const QImage& source, const QString& fileName, 
        void * clientData, int priority )
{
    int result( -1 );
    RBuf file;
    _LIT( mime, "image/png" );

    priority = convertPriority(priority);
    
    if( !byteArray ){
        byteArray = new QByteArray();
    }
    QBuffer buffer( byteArray );
    buffer.open( QBuffer::ReadWrite );
    QDataStream dataStream( &buffer );

    dataStream << source;

    int offset = ( dataStream.version() >= 5 ? 4 : 0 );
    
    TRAP_IGNORE( 
        CleanupClosePushL( file );
        file.CreateL( fileName.length() );
        file.Copy( fileName.utf16(), fileName.length() );

        HBufC* mimetype = HBufC::NewLC( 9 );
        mimetype->Des() = mime();

        TPtrC8* ptr = new TPtrC8( reinterpret_cast<const TUint8*>( byteArray->data() + offset ), byteArray->count() - offset );
                
        CThumbnailObjectSource* objSrc = CThumbnailObjectSource::NewLC( ptr, *mimetype, file );
        result = iThumbnailManager->SetThumbnailL( *objSrc, clientData, priority );
        CleanupStack::PopAndDestroy( 3, &file );
    );
    
    return result;
}

int ThumbnailManagerPrivate::setThumbnail( const QString& sourceFileName, const QString& targetFileName,
        const QString& mimeType, void * clientData, int priority )
{
    int result( -1 );
    QString symbSourceFile( sourceFileName );
    QString symbTargetFile( targetFileName );
    RBuf sourceFile;
    RBuf targetFile;
    RBuf mime;

    priority = convertPriority(priority);
    
    if( symbSourceFile.contains( "/" ) )
        symbSourceFile.replace( "/", "\\", Qt::CaseSensitive );
    
    if( symbTargetFile.contains( "/" ) )
        symbTargetFile.replace( "/", "\\", Qt::CaseSensitive );
    
    TRAP_IGNORE( 
        CleanupClosePushL( sourceFile );
        sourceFile.CreateL( symbSourceFile.length() );
        sourceFile.Copy( symbSourceFile.utf16(), symbSourceFile.length() );
        
        CleanupClosePushL( targetFile );
        targetFile.CreateL( symbTargetFile.length() );
        targetFile.Copy( symbTargetFile.utf16(), symbTargetFile.length() );
        
        CleanupClosePushL( mime );
        
        if (mimeType.length())
            {
            mime.CreateL( mimeType.length() );
            mime.Copy( mimeType.utf16(), mimeType.length() );
            }
        else
            {
            mime.Assign(KNullDesC().AllocL());
            }
                
        CThumbnailObjectSource* objSrc = CThumbnailObjectSource::NewLC( sourceFile, targetFile, mime );
        result = iThumbnailManager->SetThumbnailL( *objSrc, clientData, priority );
        CleanupStack::PopAndDestroy( 4, &sourceFile );
    );
    
    return result;
}

void ThumbnailManagerPrivate::deleteThumbnails( const QString& fileName )
{
    QString symbFileName( fileName );

    if( symbFileName.contains( "/" ) )
        symbFileName.replace( "/", "\\", Qt::CaseSensitive );

    RBuf buf;
    TRAP_IGNORE( CleanupClosePushL( buf );
        buf.CreateL( symbFileName.length() );
        buf.Copy( symbFileName.utf16(), symbFileName.length() );
        CThumbnailObjectSource* objSrc = CThumbnailObjectSource::NewL( buf, KNullDesC );
        iThumbnailManager->DeleteThumbnails( *objSrc );
        CleanupStack::PopAndDestroy( &buf );
        delete objSrc;
    );
}

void ThumbnailManagerPrivate::deleteThumbnails( unsigned long int thumbnailId )
{
    iThumbnailManager->DeleteThumbnails( thumbnailId );
}

bool ThumbnailManagerPrivate::cancelRequest( int id )
{
    return ( iThumbnailManager->CancelRequest( id ) == KErrNone );
}

bool ThumbnailManagerPrivate::changePriority( int id, int newPriority )
{
    newPriority = convertPriority(newPriority);
    
    return ( iThumbnailManager->ChangePriority( id, newPriority ) == KErrNone );
}

int ThumbnailManagerPrivate::convertPriority(int basePriority)
{
    return qBound(ThumbnailMangerPriorityLowest, basePriority, ThumbnailMangerPriorityHighest);    
}

void ThumbnailManagerPrivate::ThumbnailPreviewReady( MThumbnailData& /*aThumbnail*/,
    TThumbnailRequestId /*aId*/ )
{
}

void ThumbnailManagerPrivate::ThumbnailReady( TInt aError, MThumbnailData& aThumbnail,
    TThumbnailRequestId aId )
{
    if( byteArray ){
        delete byteArray;
        byteArray = NULL;
    }
    
    if (connectionCounterImage || connectionCounterPixmap) {
		QPixmap pixmap;

        if (aError == KErrNone) {
            pixmap = QPixmap::fromSymbianCFbsBitmap(aThumbnail.Bitmap());
        } else {
            pixmap = QPixmap(); 
        }

        if (connectionCounterImage) {
            emit q_ptr->thumbnailReady(pixmap.toImage(), aThumbnail.ClientData(), aId, aError);
        }
        
        if (connectionCounterPixmap) {
            emit q_ptr->thumbnailReady(pixmap, aThumbnail.ClientData(), aId, aError);
        }
    }
}

