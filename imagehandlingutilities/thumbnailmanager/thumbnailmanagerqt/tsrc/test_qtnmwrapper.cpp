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

    void thumbnailReadyPixmap( QPixmap , void * , int , int );

    void thumbnailReadyImage( QImage , void * , int , int );

private slots:
    void init();
    void cleanup();

    void testThumbnailReadyError();
    void testThumbnailReadyValid();
    
    void testThumbnailReadyImage();
    void testThumbnailReadyPixmap();
    void testThumbnailReadyImageAndPixmap();

    void createAndDestroy();
    void qualityPreference();
    void thumbnailSize();
    void thumbnailMode();
    
    void getThumbnailByName();
    void getThumbnailById();
    void setThumbnail();
    void setThumbnailFromPath();
    void cancelRequest();
    void changePriority();
    void deleteThumbnailsByName();
    void deleteThumbnailsById();

    void testPriorities_data();
    void testPriorities();


public:
    ThumbnailManager *wrapper;
    ThumbnailManagerPrivate *wrapper_p;
    
    QPixmap* ipixmap;
    
    int aid; 
    int aerrorCode;
    bool pixmapNull;
    
    //pixmap
    int pixmapId;
    int pixmapErr;
    void *pixmapDataPtr;
    
    //image
    int imageId;
    int imageErr;
    void *imageDataPtr;
    
};

enum testDataType{
    AllNull,
    BitmapValid
};

class TestThumbnailData : public MThumbnailData
{
public:
    static const int BITMAP_WIDTH = 26;
    static const int BITMAP_HEIGHT = 15;
    
public:
    TestThumbnailData( testDataType type, void* clientData_ = 0) : bitmap(0), clientData(clientData_)
    {
        switch( type ){
        case AllNull:
        break;

        case BitmapValid:
            bitmap = new CFbsBitmap();
            QVERIFY( !bitmap->Create(TSize(BITMAP_WIDTH, BITMAP_HEIGHT),EColor64K) );
            QVERIFY( !bitmap->Load( _L("c:\\tnmwrapper_tsrc.mbm") ) );
        break;
        };
    
    };

    ~TestThumbnailData() { 
        if( bitmap ) {
            delete bitmap;
        }
    };
    
    CFbsBitmap* Bitmap() {return bitmap;};

    CFbsBitmap* DetachBitmap() {return bitmap;};

    TAny* ClientData(){ return clientData; };

public:
    CFbsBitmap* bitmap;
    TAny* clientData;

};

// --- test initialization ---

void TestThumbnailManager::init()
{
    wrapper = new ThumbnailManager();
    connect( wrapper, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
            this, SLOT( thumbnailReady( QPixmap , void* , int , int )));
    
    wrapper_p = wrapper->d; //new ThumbnailManagerPrivate();
}
    
void TestThumbnailManager::cleanup()
{
    disconnect( wrapper, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
            this, SLOT( thumbnailReady( QPixmap , void* , int , int )));
    delete wrapper;
    wrapper = NULL;

    if( ipixmap ) {
        delete ipixmap;
        ipixmap = NULL;
        }
 
 //   delete wrapper_p;
}

// --- test ---

/**
 * Simple create and destroy.
 */
void TestThumbnailManager::createAndDestroy()
{
    //empty
}

/**
 * Checking quality preference flag.
 */
void TestThumbnailManager::qualityPreference()
{
    QVERIFY( wrapper->setQualityPreference( ThumbnailManager::OptimizeForQuality ) );
    QVERIFY( wrapper->setQualityPreference( ThumbnailManager::OptimizeForPerformance ) );
    QVERIFY( wrapper->qualityPreference() == ThumbnailManager::OptimizeForPerformance );
}

/**
 * Checking thumbnail size flag.
 */
void TestThumbnailManager::thumbnailSize()
{
    QVERIFY( wrapper->setThumbnailSize( ThumbnailManager::ThumbnailSmall ) );
    QVERIFY( wrapper->setThumbnailSize( ThumbnailManager::ThumbnailMedium ) );
    QVERIFY( wrapper->setThumbnailSize( ThumbnailManager::ThumbnailLarge ) );
    QVERIFY( wrapper->setThumbnailSize( QSize( 100, 100 ) ) );
    QVERIFY( wrapper->thumbnailSize() == QSize( 100, 100 ) );
}

/**
 * Checking thumbnail mode flag.
 */
void TestThumbnailManager::thumbnailMode()
{
    QVERIFY( wrapper->setMode( ThumbnailManager::Default ) );
    QVERIFY( wrapper->setMode( ThumbnailManager::AllowAnySize ) );
    QVERIFY( wrapper->setMode( ThumbnailManager::DoNotCreate ) );
    
    QVERIFY( wrapper->setMode( ThumbnailManager::CropToAspectRatio ) );
    QVERIFY( wrapper->mode() == ThumbnailManager::CropToAspectRatio );
}

/**
 * Checking thumbnail mode flag.
 */
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

void TestThumbnailManager::setThumbnailFromPath()
{ 
    // normal URI
    QVERIFY( wrapper->setThumbnail( "c:\\tnmwrapper_tsrc.png", "c:\\tnmwrapper_tsrc.vcf", "contact/x-vcard" ) != -1 );
    
    // virtual URI
    QVERIFY( wrapper->setThumbnail( "c:\\tnmwrapper_tsrc.png", "c:tnmwrapper_tsrc.vcf", "contact/x-vcard" ) != -1 );
    
    // no mime
    QVERIFY( wrapper->setThumbnail( "c:\\tnmwrapper_tsrc.png", "c:tnmwrapper_tsrc.vcf" ) != -1 );
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

void TestThumbnailManager::testThumbnailReadyError()
{
    int a=0;

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
}

void TestThumbnailManager::testThumbnailReadyValid()
{
    QVERIFY( connect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReady_p( QPixmap , void* , int , int )), Qt::DirectConnection ) );

    TestThumbnailData tdata3( BitmapValid );
    aid = 10; 
    aerrorCode = 0;
    pixmapNull = false;
    wrapper_p->ThumbnailReady( aerrorCode, tdata3, aid );

    disconnect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReady_p( QPixmap , void* , int , int )));
}

void TestThumbnailManager::testThumbnailReadyImage()
{
    QVERIFY( connect( wrapper_p, SIGNAL(thumbnailReady( QImage , void* , int, int ) ),
        this, SLOT( thumbnailReadyImage( QImage , void* , int , int )), Qt::DirectConnection ) );

    pixmapId = 0;
    pixmapErr = 0;
    pixmapDataPtr = 0;

    imageId = 432;
    imageErr = KErrNone;
    imageDataPtr = reinterpret_cast<void*>(0x1234);
    
    TestThumbnailData td = TestThumbnailData(BitmapValid);
    td.clientData = imageDataPtr;

    wrapper_p->ThumbnailReady(imageErr, td, imageId);
    
    QVERIFY(td.bitmap!=0);
    QVERIFY(td.bitmap->SizeInPixels().iWidth == TestThumbnailData::BITMAP_WIDTH);
    QVERIFY(td.bitmap->SizeInPixels().iHeight == TestThumbnailData::BITMAP_HEIGHT);

    disconnect( wrapper_p, SIGNAL(thumbnailReady( QImage , void* , int, int ) ),
        this, SLOT( thumbnailReadyImage(QImage, void* , int , int )));  
}

void TestThumbnailManager::testThumbnailReadyPixmap()
{
    QVERIFY( connect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReadyPixmap( QPixmap , void* , int , int )), Qt::DirectConnection ) );

    pixmapId = 432;
    pixmapErr = -7;
    pixmapDataPtr = reinterpret_cast<void*>(0x1234);

    imageId = 0;
    imageErr = 0;
    imageDataPtr = 0;

    TestThumbnailData td = TestThumbnailData(BitmapValid);
    td.clientData = pixmapDataPtr;

    wrapper_p->ThumbnailReady(pixmapErr, td, pixmapId);
    
    QVERIFY(td.bitmap!=0);
    QVERIFY(td.bitmap->SizeInPixels().iWidth == TestThumbnailData::BITMAP_WIDTH);
    QVERIFY(td.bitmap->SizeInPixels().iHeight == TestThumbnailData::BITMAP_HEIGHT);

    disconnect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReadyPixmap( QPixmap , void* , int , int )));
}

void TestThumbnailManager::testThumbnailReadyImageAndPixmap()
{
    QVERIFY( connect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReadyPixmap( QPixmap , void* , int , int )), Qt::DirectConnection ) );

    QVERIFY( connect( wrapper_p, SIGNAL(thumbnailReady( QImage , void* , int, int ) ),
        this, SLOT( thumbnailReadyImage( QImage , void* , int , int )), Qt::DirectConnection ) );

    pixmapId = 432;
    pixmapErr = KErrNone;
    pixmapDataPtr = reinterpret_cast<void*>(0x1234);

    imageId = pixmapId;
    imageErr = pixmapErr;
    imageDataPtr = pixmapDataPtr;

    TestThumbnailData td = TestThumbnailData(BitmapValid);
    td.clientData = pixmapDataPtr;

    wrapper_p->ThumbnailReady(pixmapErr, td, pixmapId);

    QVERIFY(td.bitmap!=0);
    QVERIFY(td.bitmap->SizeInPixels().iWidth == TestThumbnailData::BITMAP_WIDTH);
    QVERIFY(td.bitmap->SizeInPixels().iHeight == TestThumbnailData::BITMAP_HEIGHT);

    disconnect( wrapper_p, SIGNAL(thumbnailReady( QImage , void* , int, int ) ),
        this, SLOT( thumbnailReadyImage( QImage , void* , int , int )));

    disconnect( wrapper_p, SIGNAL(thumbnailReady( QPixmap , void* , int, int ) ),
        this, SLOT( thumbnailReadyPixmap( QPixmap , void* , int , int )));    
}

void TestThumbnailManager::testPriorities_data()
{
    // note: current bounds for priorites are [-100:20] (from: CActive::TPriority)
    
    QTest::addColumn<int>("priority");
    QTest::addColumn<int>("expectedPriority");
    QTest::newRow("priority 0") << 0 << 0;
    QTest::newRow("priority 10") << 10 << 10;
    QTest::newRow("priority 20") << 20 << 20;
    QTest::newRow("priority 100") << 100 << 20;
    QTest::newRow("priority 200") << 200 << 20;
    QTest::newRow("priority -10") << -10 << -10;
    QTest::newRow("priority -100") << -100 << -100;
    QTest::newRow("priority -200") << -200 << -100;
}

void TestThumbnailManager::testPriorities()
{
    QFETCH(int, priority);
    QFETCH(int, expectedPriority);
    QCOMPARE(wrapper_p->convertPriority(priority), expectedPriority);
}
 

// --- private slots - callback  ---

void TestThumbnailManager::thumbnailReady( QPixmap pixmap, void *clientData, int id, int errorCode )
{
    //do nothing, we dont test Thumbnail Manager's functionality, we just use it
    Q_UNUSED( pixmap );
    Q_UNUSED( clientData );
    Q_UNUSED( id );
    Q_UNUSED( errorCode );
}

void TestThumbnailManager::thumbnailReady_p( QPixmap pixmap, void * clientData, int id, int errorCode )
{
    Q_UNUSED( clientData );
    QVERIFY( pixmap.isNull() == pixmapNull );
    QVERIFY( errorCode == aerrorCode );
    QVERIFY( id == aid );
}

void TestThumbnailManager::thumbnailReadyPixmap( QPixmap pixmap, void *clientData, int id, int errorCode )
{
    Q_UNUSED( pixmap );
    QVERIFY( errorCode == pixmapErr );
    QVERIFY( id == pixmapId );
    QVERIFY( clientData = pixmapDataPtr );
}

void TestThumbnailManager::thumbnailReadyImage( QImage image, void *clientData, int id, int errorCode )
{
    Q_UNUSED( image );
    QVERIFY( errorCode == imageErr );
    QVERIFY( id == imageId );
    QVERIFY( clientData = imageDataPtr );
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
    int main (int argc, char* argv[]) 
    {
        for ( int i=0;i<argc; i++){
            if (strcmp(argv[i], "-o")==0 && i+1 <argc ){
                //let's make sure that folder specified after -o exists
                QDir file( QString::fromLatin1( argv[i+1] ));
                QString s = file.absolutePath ();
                s = s.left( s.lastIndexOf(file.dirName()) );
                if ( !file.exists(s) ){
                    file.mkpath(s);
                }
            }
        }
        
        QApplication app(argc, argv);
        QTEST_DISABLE_KEYPAD_NAVIGATION
        TestThumbnailManager tc;
        return QTest::qExec(&tc, argc, argv);
    }
#endif
	
#include "test_qtnmwrapper.moc"

