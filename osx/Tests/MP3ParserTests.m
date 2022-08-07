//
//  MP3ParserTest.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 6/14/18.
//  Copyright © 2018 Oleksiy Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "mp3parser.h"
#include "../../common.h"
#include "vfs.h"

@interface MP3ParserTest : XCTestCase

@end

@implementation MP3ParserTest

- (void)setUp {
    [super setUp];
}

- (void)tearDown {
    [super tearDown];
}

- (void)test_2secSquareWithLameHeader_XingDetectedWith88200Samples {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 1);
    XCTAssertEqual(info.ref_packet.samplerate, 44100);
    XCTAssertEqual(info.totalsamples-info.delay-info.padding, 88200);
    XCTAssertEqual(info.delay, 1105);
    XCTAssertEqual(info.padding, 551);
    XCTAssertEqual(info.lame_musiclength, 16508);
}

- (void)test_2secSquareWithLameHeader_FullScanGet88200Samples {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_FULLSCAN, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 1);
    XCTAssertEqual(info.ref_packet.samplerate, 44100);
    XCTAssertEqual(info.totalsamples, 88200+576+1080);
    XCTAssertEqual(info.pcmsample, 0);
    XCTAssertEqual(info.delay, 529);
    XCTAssertEqual(info.padding, 0);
    XCTAssertEqual(info.lame_musiclength, 16508);
}

- (void)test_2secSquareSeekTo0_GivesPacketAfterXingPos208 {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 0);
    XCTAssert (!res);
    XCTAssertEqual(info.packet_offs, 208);
}

- (void)test_2secSquareSeekToDelay_GivesPacketAfterXingPos208 {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576);
    XCTAssert (!res);
    XCTAssertEqual(info.packet_offs, 208);
}

- (void)test_2secSquareSeekTo1sec_SeeksTo1SecMinus10Packets {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576+44100);
    XCTAssert (!res);
    XCTAssertEqual(info.packet_offs, 6059);
    XCTAssertEqual(info.pcmsample, 32256);
}

- (void)test_2secSquareNoLameHeader_Gives88200Samples {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-nolamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 0);
    XCTAssertEqual(info.ref_packet.samplerate, 44100);
    // lame adds default encoder delay and padding of 576 and 1080, even without header
    XCTAssertEqual(info.totalsamples, 88200+576+1080);
    XCTAssertEqual(info.pcmsample, 0);
    XCTAssertEqual(info.delay, 529);
    XCTAssertEqual(info.padding, 0);
}

- (void)test_2secSquareNoXHSeekTo1sec_SeeksTo1SecMinus10Packets {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-nolamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576+44100);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 0);
    XCTAssertEqual(info.packet_offs, 5851);
    XCTAssertEqual(info.pcmsample, 32256);
}

// the file contains garbage/invalid data around the middle of the file, with packet markers.
// we still expect the parser to deal with it
- (void)test_2secSquareWithGarbage_Reports88200SamplesLength {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-nolamehdr-garbage.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 0);
    XCTAssertEqual(info.ref_packet.samplerate, 44100);
    // lame adds default encoder delay and padding of 576 and 1080, even without header
    XCTAssertEqual(info.totalsamples, 88200+576+1080);
    XCTAssertEqual(info.pcmsample, 0);
    XCTAssertEqual(info.delay, 529);
    XCTAssertEqual(info.padding, 0);
}

- (void)test_FiniteCBRNetworkStream_ReportsAccurateDuration {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/cbr_rhytm_30sec.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 0);
    XCTAssertEqual(info.ref_packet.samplerate, 32000);
    XCTAssertEqual(info.totalsamples, 1025280);
    XCTAssertEqual(info.npackets, 890);
    XCTAssertLessThan(info.valid_packets, info.npackets);
}

- (void)test_FiniteCBRLameHdrNetworkStream_ReportsAccurateDuration {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/cbr_rhytm_30sec_lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 1);
    XCTAssertEqual(info.ref_packet.samplerate, 32000);
    XCTAssertEqual(info.totalsamples, 1025280);
    XCTAssertEqual(info.npackets, 890);
    XCTAssertLessThan(info.valid_packets, info.npackets);
}

- (void)test_FiniteVBRNetworkStream_ReportsApproximateDuration {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/vbr_rhytm_30sec.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 0);
    XCTAssertEqual(info.ref_packet.samplerate, 32000);
    XCTAssertEqualWithAccuracy(info.totalsamples, 1025280, 2000);
    XCTAssertEqualWithAccuracy(info.npackets, 890, 10);
    XCTAssertLessThan(info.valid_packets, info.npackets);
}

- (void)test_FiniteVBRLameHdrNetworkStream_ReportsAccurateDuration {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/vbr_rhytm_30sec_lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_ESTIMATE_DURATION, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 1);
    XCTAssertEqual(info.ref_packet.samplerate, 32000);
    XCTAssertEqual(info.totalsamples, 1025280);
    XCTAssertEqualWithAccuracy(info.npackets, 890, 10);
    XCTAssertLessThan(info.valid_packets, info.npackets);
}

@end
