#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/io/catchiofailure.h>
#include <c++utilities/tests/testutils.h>

#include <tagparser/mediafileinfo.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include <iostream>
#include <fstream>

using namespace std;
using namespace TestUtilities;
using namespace ConversionUtilities;
using namespace Media;

using namespace CPPUNIT_NS;

enum class TagStatus
{
    Original,
    TestMetaDataPresent,
    Removed
};

/*!
 * \brief The CliTests class tests the command line interface.
 */
class CliTests : public TestFixture
{
    CPPUNIT_TEST_SUITE(CliTests);
#ifdef PLATFORM_UNIX
    CPPUNIT_TEST(testBasicReadingAndWriting);
    CPPUNIT_TEST(testHandlingOfTargets);
    CPPUNIT_TEST(testHandlingOfId3Tags);
    CPPUNIT_TEST(testMultipleFiles);
    CPPUNIT_TEST(testMultipleValuesPerField);
    CPPUNIT_TEST(testHandlingAttachments);
    CPPUNIT_TEST(testDisplayingInfo);
    CPPUNIT_TEST(testExtraction);
    CPPUNIT_TEST(testReadingAndWritingDocumentTitle);
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

#ifdef PLATFORM_UNIX
    void testBasicReadingAndWriting();
    void testHandlingOfTargets();
    void testHandlingOfId3Tags();
    void testMultipleFiles();
    void testMultipleValuesPerField();
    void testHandlingAttachments();
    void testDisplayingInfo();
    void testExtraction();
    void testReadingAndWritingDocumentTitle();
#endif

private:

};

CPPUNIT_TEST_SUITE_REGISTRATION(CliTests);

void CliTests::setUp()
{}

void CliTests::tearDown()
{}

#ifdef PLATFORM_UNIX
/*!
 * \brief Tests basic reading and writing of tags.
 */
void CliTests::testBasicReadingAndWriting()
{
    string stdout, stderr;
    // get specific field
    const string mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));
    const string mkvFileBackup(mkvFile + ".bak");
    const char *const args1[] = {"tageditor", "get", "title", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    // context of the following fields is the album (so "Title" means the title of the album)
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "album",
                                          "Title             Elephant Dream - test 2"
                                      }));
    CPPUNIT_ASSERT(stdout.find("Year              2010") == string::npos);

    // get all fields
    const char *const args2[] = {"tageditor", "get", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Title             Elephant Dream - test 2",
                                          "Year              2010",
                                          "Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC"
                                      }));

    // set some fields, keep other field
    const char *const args3[] = {"tageditor", "set", "title=A new title", "genre=Testfile", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args3, stdout, stderr));
    CPPUNIT_ASSERT(stdout.find("Changes have been applied") != string::npos);
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Title             A new title",
                                          "Genre             Testfile",
                                          "Year              2010",
                                          "Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC"
                                      }));
    // clear backup file
    remove(mkvFileBackup.data());

    // set some fields, discard other
    const char *const args4[] = {"tageditor", "set", "title=Foo", "artist=Bar", "--remove-other-fields", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args4, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Title             Foo",
                                          "Artist            Bar"
                                      }));
    CPPUNIT_ASSERT(stdout.find("Year") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Comment") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Genre") == string::npos);

    // clear working copies if all tests have been
    remove(mkvFile.c_str());
    remove(mkvFileBackup.data());
}

/*!
 * \brief Tests adding and removing of targets.
 */
void CliTests::testHandlingOfTargets()
{
    string stdout, stderr;
    const string mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));
    const string mkvFileBackup(mkvFile + ".bak");
    const char *const args1[] = {"tageditor", "get", "-f", mkvFile.data(), nullptr};

    // add song title (title field for tag with level 30)
    const char *const args2[] = {"tageditor", "set", "target-level=30", "title=The song title", "genre=The song genre", "target-level=50", "genre=The album genre", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "song",
                                          "Title             The song title",
                                          "Genre             The song genre"
                                      }));
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "album",
                                          "Title             Elephant Dream - test 2",
                                          "Genre             The album genre"
                                      }));
    remove(mkvFileBackup.data());

    // remove tags targeting level 30 and 50 and add new tag targeting level 30 and the audio track
    const char *const args3[] = {"tageditor", "set", "target-level=30", "target-tracks=3134325680", "title=The audio track", "encoder=likely some AAC encoder", "--remove-target", "target-level=30", "--remove-target", "target-level=50", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args3, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(containsSubstrings(stdout, {"song"}));
    CPPUNIT_ASSERT(!containsSubstrings(stdout, {"song", "song"}));
    CPPUNIT_ASSERT(stdout.find("album") == string::npos);
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "3134325680",
                                          "Title             The audio track",
                                          "Encoder           likely some AAC encoder"
                                      }));
    remove(mkvFileBackup.data());

    // clear working copies if all tests have been
    remove(mkvFile.c_str());
}

/*!
 * \brief Tests handling of ID3v1 and ID3v2 tags and MP3 specific options.
 */
void CliTests::testHandlingOfId3Tags()
{
    // TODO
}

/*!
 * \brief Tests reading and writing multiple files at once.
 */
void CliTests::testMultipleFiles()
{
    string stdout, stderr;
    const string mkvFile1(workingCopyPath("matroska_wave1/test1.mkv"));
    const string mkvFile2(workingCopyPath("matroska_wave1/test2.mkv"));
    const string mkvFile3(workingCopyPath("matroska_wave1/test3.mkv"));

    // get tags of 3 files at once
    const char *const args1[] = {"tageditor", "get", "-f", mkvFile1.data(), mkvFile2.data(), mkvFile3.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Title             Big Buck Bunny - test 1",
                                          "Title             Elephant Dream - test 2",
                                          "Title             Elephant Dream - test 3"
                                      }));
    // clear backup files
    remove((mkvFile1 + ".bak").c_str()), remove((mkvFile2 + ".bak").c_str()), remove((mkvFile3 + ".bak").c_str());

    // set title and part number of 3 files at once
    const char *const args2[] = {"tageditor", "set", "target-level=30", "title=test1", "title=test2", "title=test3", "part+=1", "target-level=50", "title=MKV testfiles", "totalparts=3", "-f", mkvFile1.data(), mkvFile2.data(), mkvFile3.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Matroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\n"
                                          " Title             MKV testfiles\n"
                                          " Year              2010\n"
                                          " Comment           Matroska Validation File1, basic MPEG4.2 and MP3 with only SimpleBlock\n"
                                          " Total parts       3\n"
                                          "Matroska tag targeting \"level 30 'track, song, chapter'\"\n"
                                          " Title             test1\n"
                                          " Part              1",
                                          "Matroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\n"
                                          " Title             MKV testfiles\n"
                                          " Year              2010\n"
                                          " Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC\n"
                                          " Total parts       3\n"
                                          "Matroska tag targeting \"level 30 'track, song, chapter'\"\n"
                                          " Title             test2\n"
                                          " Part              2",
                                          "Matroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\n"
                                          " Title             MKV testfiles\n"
                                          " Year              2010\n"
                                          " Comment           Matroska Validation File 3, header stripping on the video track and no SimpleBlock\n"
                                          " Total parts       3\n"
                                          "Matroska tag targeting \"level 30 'track, song, chapter'\"\n"
                                          " Title             test3\n"
                                          " Part              3"
                                      }));

    // clear working copies if all tests have been
    remove(mkvFile1.c_str()), remove(mkvFile2.c_str()), remove(mkvFile3.c_str());
    remove((mkvFile1 + ".bak").c_str()), remove((mkvFile2 + ".bak").c_str()), remove((mkvFile3 + ".bak").c_str());
}

/*!
 * \brief Tests tagging multiple values per field.
 * \remarks Fails because feature has not been implemented yet.
 */
void CliTests::testMultipleValuesPerField()
{
    string stdout, stderr;
    const string mkvFile(workingCopyPath("matroska_wave1/test1.mkv"));
    const char *const args1[] = {"tageditor", "get", "-f", mkvFile.data(), nullptr};
    const char *const args2[] = {"tageditor", "set", "artist=test1", "+artist=test2", "+artist=test3", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    //cout << stdout << endl;
    //cerr << stderr << endl;
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Artist            test1",
                                          "Artist            test2",
                                          "Artist            test3"
                                      }));

    // clear working copies if all tests have been
    remove(mkvFile.c_str());
    remove((mkvFile + ".bak").c_str());
}

/*!
 * \brief Tests handling attachments.
 */
void CliTests::testHandlingAttachments()
{
    string stdout, stderr;
    const string mkvFile1(workingCopyPath("matroska_wave1/test1.mkv"));
    const string mkvFile1Backup(mkvFile1 + ".bak");
    const string mkvFile2("path=" + testFilePath("matroska_wave1/test2.mkv"));

    // add attachment
    const char *const args2[] = {"tageditor", "set", "--add-attachment", "name=test2.mkv", "mime=video/x-matroska", "desc=Test attachment", mkvFile2.data(), "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    const char *const args1[] = {"tageditor", "info", "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Attachments:",
                                          "Name                          test2.mkv",
                                          "MIME-type                     video/x-matroska",
                                          "Description                   Test attachment",
                                          "Size                          20.16 MiB (21142764 byte)"
                                      }));
    // clear backup file
    remove(mkvFile1Backup.data());

    // update attachment
    const char *const args3[] = {"tageditor", "set", "--update-attachment", "name=test2.mkv", "desc=Updated test attachment", "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args3, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(containsSubstrings(stdout, {
                                          "Attachments:",
                                          "Name                          test2.mkv",
                                          "MIME-type                     video/x-matroska",
                                          "Description                   Updated test attachment",
                                          "Size                          20.16 MiB (21142764 byte)"
                                      }));
    // clear backup file
    remove(mkvFile1Backup.data());

    // extract assigned attachment again
    const char *const args4[] = {"tageditor", "extract", "--attachment", "name=test2.mkv", "-f", mkvFile1.data(), "-o", "/tmp/extracted.mkv", nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args4, stdout, stderr));
    fstream origFile, extFile;
    origFile.exceptions(ios_base::failbit | ios_base::badbit), extFile.exceptions(ios_base::failbit | ios_base::badbit);
    origFile.open(mkvFile2.data() + 5, ios_base::in | ios_base::binary), extFile.open("/tmp/extracted.mkv", ios_base::in | ios_base::binary);
    origFile.seekg(0, ios_base::end), extFile.seekg(0, ios_base::end);
    int64 origFileSize = origFile.tellg(), extFileSize = extFile.tellg();
    CPPUNIT_ASSERT_EQUAL(origFileSize, extFileSize);
    for(origFile.seekg(0), extFile.seekg(0); origFileSize > 0; --origFileSize) {
        CPPUNIT_ASSERT_EQUAL(origFile.get(), extFile.get());
    }
    remove("/tmp/extracted.mkv");

    // remove assigned attachment
    const char *const args5[] = {"tageditor", "set", "--remove-attachment", "name=test2.mkv", "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args5, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(stdout.find("Attachments:") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Name                          test2.mkv") == string::npos);

    // clear working copies if all tests have been
    remove(mkvFile1.data());
    remove(mkvFile1Backup.data());
}

/*!
 * \brief Tests displaying general file info.
 */
void CliTests::testDisplayingInfo()
{
    // TODO (not very important)
}

/*!
 * \brief Tests extraction of field values (used to extract cover or other binary fields).
 * \remarks Extraction of attachments is already tested in testHandlingAttachments().
 */
void CliTests::testExtraction()
{
    string stdout, stderr;
    const string mp41File(testFilePath("mtx-test-data/alac/othertest-itunes.m4a"));

    // test extraction of cover
    const char *const args1[] = {"tageditor", "extract", "cover", "-f", mp41File.data(), "-o", "/tmp/extracted.jpeg", nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    MediaFileInfo extractedInfo("/tmp/extracted.jpeg");
    extractedInfo.open(true);
    extractedInfo.parseContainerFormat();
    CPPUNIT_ASSERT_EQUAL(22771ul, extractedInfo.size());
    CPPUNIT_ASSERT(ContainerFormat::Jpeg == extractedInfo.containerFormat());
    extractedInfo.invalidate();

    // test assignment of cover by the way
    const string mp4File2(workingCopyPath("mtx-test-data/aac/he-aacv2-ps.m4a"));
    const char *const args2[] = {"tageditor", "set", "cover=/tmp/extracted.jpeg", "-f", mp4File2.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    const char *const args3[] = {"tageditor", "extract", "cover", "-f", mp4File2.data(), "-o", "/tmp/extracted.jpeg", nullptr};
    remove("/tmp/extracted.jpeg");
    CPPUNIT_ASSERT_EQUAL(0, execApp(args3, stdout, stderr));
    extractedInfo.open(true);
    extractedInfo.parseContainerFormat();
    CPPUNIT_ASSERT_EQUAL(22771ul, extractedInfo.size());
    CPPUNIT_ASSERT(ContainerFormat::Jpeg == extractedInfo.containerFormat());
    remove("/tmp/extracted.jpeg");
    remove(mp4File2.data());
    remove((mp4File2 + ".bak").data());
}

/*!
 * \brief Tests reading and writing the document title.
 */
void CliTests::testReadingAndWritingDocumentTitle()
{
    // TODO
}
#endif
