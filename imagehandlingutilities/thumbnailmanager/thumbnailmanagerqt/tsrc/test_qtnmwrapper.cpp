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

#include <QtGui>
#include <QtTest/QtTest>
#include <QEventLoop>
#include <QDebug>
#include <QImage>
#include <fbs.h>

#include <thumbnaildata.h>

#include <thumbnailmanager_qt.h>
#include "thumbnailmanager_p_qt.h"

class TestThumbnailManager : public QObject
{
    Q_OBJECT

public:
    TestThumbnailManager(): QObject(), wrapper( NULL ), ipixmap( NULL ) {};

public slots:
    void thumbnailReady( QPixmap , void * , int , int );

    void thumbnailReady_p( QPixmap , void * , int , int );

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void createAndDestroy();
    void qualityPreference();
    void thumbnailSize();
    void thumbnailMode();
    
    void getThumbnailByName();
    void getThumbnailById();
    void setThumbnail();
    void cancelRequest();
    void changePriority();
    void deleteThumbnailsByName();
    void deleteThumbnailsById();

    void testThumbnailReadyError();
    void testThumbnailReadyValid();

public:
    ThumbnailManager* wrapper;
    QPixmap* ipixmap;
    
    int aid; 
    int aerrorCode;
    bool pixmapNull;
    
};

enum testDataType{
    AllNull,
    BitmapValid
};

class TestThumbnailData : public MThumbnailData
{
public:
    TestThumbnailData( testDataType type) : bitmap(0), clientData(0)
    {
        switch( type ){
        case AllNull:
        break;

        case BitmapValid:
            bitmap = new CFbsBitmap();
            QVERIFY( !bitmap->Create(TSize(26,15),EColor64K) );
            QVERIFY( !bitmap->Load( _L("c:\\tnmwrapper_tsrc.mbm") ) );
        break;
        };
    
    };

    ~TestThumbnailData() { 
        if( bitmap )
            delete bitmap; 
        if( clientData )
            delete clientData; 
    };
    
    CFbsBitmap* Bitmap() {return bitmap;};

    CFbsBitmap* DetachBitmap() {return bitmap;};

    TAny* ClientData(){ return clientData; };

public:
    CFbsBitmap* bitmap;
    TAny* clientData;

};


// ======== MEMBER FUNCTIONS ========
void TestThumbnailManager::initTestCase()
{
    wrapper = new ThumbnailManager();
    connect( wrapper, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
            this, SLOT( thumbnailReady( QPixmap , void* , int , int )));
}
    
void TestThumbnailManager::cleanupTestCase()
{
    disconnect( wrapper, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
            this, SLOT( thumbnailReady( QPixmap , void* , int , int )));
    delete wrapper;
    wrapper = NULL;

    if( ipixmap ){
        delete ipixmap;
        ipixmap = NULL;
        }
}


void TestThumbnailManager::createAndDestroy()
{
    //empty
}

void TestThumbnailManager::qualityPreference()
{
    QVERIFY( wrapper->setQualityPreference( ThumbnailManager::OptimizeForQuality ) );
    QVERIFY( wrapper->setQualityPreference( ThumbnailManager::OptimizeForPerformance ) );
    QVERIFY( wrapper->qualityPreference() == ThumbnailManager::OptimizeForPerformance );
}

void TestThumbnailManager::thumbnailSize()
{
    QVERIFY( wrapper->setThumbnailSize( ThumbnailManager::ThumbnailSmall ) );
    QVERIFY( wrapper->setThumbnailSize( ThumbnailManager::ThumbnailMedium ) );
    QVERIFY( wrapper->setThumbnailSize( ThumbnailManager::ThumbnailLarge ) );
    QVERIFY( wrapper->setThumbnailSize( QSize( 100, 100 ) ) );
    QVERIFY( wrapper->thumbnailSize() == QSize( 100, 100 ) );
}

void TestThumbnailManager::thumbnailMode()
{
    QVERIFY( wrapper->setMode( ThumbnailManager::Default ) );
    QVERIFY( wrapper->setMode( ThumbnailManager::AllowAnySize ) );
    QVERIFY( wrapper->setMode( ThumbnailManager::DoNotCreate ) );
    
    QVERIFY( wrapper->setMode( ThumbnailManager::CropToAspectRatio ) );
    QVERIFY( wrapper->mode() == ThumbnailManager::CropToAspectRatio );
}

void TestThumbnailManager::getThumbnailByName()
{
    wrapper->setMode( ThumbnailManager::CropToAspectRatio );
    wrapper->setThumbnailSize( QSize( 200, 50 )); 
    QVERIFY( wrapper->getThumbnail( "c:\\tnmwrapper_tsrc.png", NULL, -99 ) != -1 );
    QVERIFY( wrapper->getThumbnail( "c:/tnmwrapper_tsrc.png", NULL, -99 ) != -1 );
}

void TestThumbnailManager::getThumbnailById()
{
    wrapper->setMode( ThumbnailManager::CropToAspectRatio );
    wrapper->setThumbnailSize( QSize( 200, 50 )); 
    QVERIFY( wrapper->getThumbnail( 2, NULL, -99 ) != -1 );
}

void TestThumbnailManager::setThumbnail()
{
    ipixmap = new QPixmap();
    ipixmap->load( "c:\\tnmwrapper.bmp" );
    wrapper->setMode( ThumbnailManager::CropToAspectRatio );
    wrapper->setThumbnailSize(ThumbnailManager::ThumbnailMedium); 
    QVERIFY( wrapper->setThumbnail( *ipixmap, "c:\\tnmwrapper_tsrc.png" ) != -1 );
    ipixmap->fill();
    QVERIFY( wrapper->setThumbnail( *ipixmap, "c:\\tnmwrapper_tsrc.png" ) != -1 );

	//QI,mage
	QImage *img = new QImage("c:\\tnmwrapper.bmp");
    QVERIFY( wrapper->setThumbnail( *img, "c:\\tnmwrapper_tsrc.png" ) != -1 );
    img->fill(0);
    QVERIFY( wrapper->setThumbnail( *img, "c:\\tnmwrapper_tsrc.png" ) != -1 );
	delete img;
}

void TestThumbnailManager::cancelRequest()
{
    wrapper->setMode( ThumbnailManager::CropToAspectRatio );
    wrapper->setThumbnailSize( QSize( 200, 50 )); 
    QVERIFY( wrapper->cancelRequest( wrapper->getThumbnail( "c:\\tnmwrapper_tsrc.png", NULL, -99 ) ) );
    QVERIFY( !wrapper->cancelRequest( 123 ) ); //test request not found
}

void TestThumbnailManager::changePriority()
{
    wrapper->setMode( ThumbnailManager::CropToAspectRatio );
    wrapper->setThumbnailSize( QSize( 200, 50 )); 
    QVERIFY( wrapper->changePriority( wrapper->getThumbnail( "c:\\tnmwrapper_tsrc.png", NULL, -99 ), -80 ) );
}

void TestThumbnailManager::deleteThumbnailsByName()
{
    wrapper->deleteThumbnails( "c:/tnmwrapper_tsrc.png" );
    wrapper->deleteThumbnails( "c:\\tnmwrapper_tsrc.png" );
}

void TestThumbnailManager::deleteThumbnailsById()
{
    wrapper->deleteThumbnails( 2 );
}

void TestThumbnailManager::thumbnailReady( QPixmap /*pixmap*/, void * /*clientData*/, int /*id*/, int /*errorCode*/ )
{
    //do nothing, we dont test Thumbnail Manager's functionality, we just use it
}

void TestThumbnailManager::thumbnailReady_p( QPixmap pixmap, void * /*clientData*/, int id, int errorCode )
{
    QVERIFY( pixmap.isNull() == pixmapNull );
    QVERIFY( errorCode == aerrorCode );
    QVERIFY( aid == id );
}

void TestThumbnailManager::testThumbnailReadyError()
{
    ThumbnailManagerPrivate* wrapper_p = new ThumbnailManagerPrivate();

    QVERIFY( connect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReady_p( QPixmap , void* , int , int )), Qt::DirectConnection ) );

    //test bytearray not null and thumbnail error
    TestThumbnailData tdata1(AllNull);
    aid = 12; 
    aerrorCode = -1;
    pixmapNull = true;
    wrapper_p->ThumbnailReady( aerrorCode, tdata1, aid );
    
    disconnect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReady_p( QPixmap , void* , int , int )));
    delete wrapper_p;
}

void TestThumbnailManager::testThumbnailReadyValid()
{
    ThumbnailManagerPrivate* wrapper_p = new ThumbnailManagerPrivate();

    QVERIFY( connect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReady_p( QPixmap , void* , int , int )), Qt::DirectConnection ) );

    TestThumbnailData tdata3( BitmapValid );
    aid = 10; 
    aerrorCode = 0;
    pixmapNull = false;
    wrapper_p->ThumbnailReady( aerrorCode, tdata3, aid );

    disconnect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReady_p( QPixmap , void* , int , int )));
    delete wrapper_p;
}

#ifdef _LOG_TO_C_
    int main (int argc, char* argv[]) 
    {
        QApplication app(argc, argv);
        TestThumbnailManager tc;
        int c = 3;
        char* v[] = {argv[0], "-o", "c:/test.txt"};
        return QTest::qExec(&tc, c, v);
    }
#else
    QTEST_MAIN(TestThumbnailManager)
#endif
	
#include "test_qtnmwrapper.moc"

