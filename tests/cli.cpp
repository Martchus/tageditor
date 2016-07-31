#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/tests/testutils.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include <iostream>

using namespace std;
using namespace TestUtilities;

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
    const char *const args1[] = {"tageditor", "get", "title", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    // context of the following fields is the album (so "Title" means the title of the album)
    CPPUNIT_ASSERT(stdout.find("album") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Title             Elephant Dream - test 2") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Year              2010") == string::npos);

    // get all fields
    const char *const args2[] = {"tageditor", "get", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(stdout.find("Title             Elephant Dream - test 2") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Year              2010") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC") != string::npos);

    // set some fields, keep other field
    const char *const args3[] = {"tageditor", "set", "title=A new title", "genre=Testfile", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args3, stdout, stderr));
    CPPUNIT_ASSERT(stdout.find("Changes have been applied") != string::npos);
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(stdout.find("Title             A new title") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Year              2010") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Genre             Testfile") != string::npos);

    // set some fields, discard other
    const char *const args4[] = {"tageditor", "set", "title=Foo", "artist=Bar", "--remove-other-fields", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args4, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT(stderr.empty());
    CPPUNIT_ASSERT(stdout.find("Title             Foo") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Artist            Bar") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Year") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Comment") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Genre") == string::npos);
}

/*!
 * \brief Tests adding and removing of targets.
 */
void CliTests::testHandlingOfTargets()
{
    string stdout, stderr;
    const string mkvFile(workingCopyPath("matroska_wave1/test2.mkv"));
    const char *const args1[] = {"tageditor", "get", "-f", mkvFile.data(), nullptr};

    // add song title (title field for tag with level 30)
    const char *const args2[] = {"tageditor", "set", "target-level=30", "title=The song title", "genre=The song genre", "target-level=50", "genre=The album genre", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    size_t songPos, albumPos;
    CPPUNIT_ASSERT((songPos = stdout.find("song")) != string::npos);
    CPPUNIT_ASSERT((albumPos = stdout.find("album")) != string::npos);
    CPPUNIT_ASSERT(stdout.find("Title             The song title") > songPos);
    CPPUNIT_ASSERT(stdout.find("Genre             The song genre") > songPos);
    CPPUNIT_ASSERT(stdout.find("Title             Elephant Dream - test 2") > albumPos);
    CPPUNIT_ASSERT(stdout.find("Genre             The album genre") > albumPos);

    // remove tags targeting level 30 and 50 and add new tag targeting level 30 and the audio track
    const char *const args3[] = {"tageditor", "set", "target-level=30", "target-tracks=3134325680", "title=The audio track", "encoder=likely some AAC encoder", "--remove-target", "target-level=30", "--remove-target", "target-level=50", "-f", mkvFile.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args3, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT((songPos = stdout.find("song")) != string::npos);
    CPPUNIT_ASSERT(stdout.find("song", songPos + 1) == string::npos);
    CPPUNIT_ASSERT(stdout.find("3134325680") != string::npos);
    CPPUNIT_ASSERT((albumPos = stdout.find("album")) == string::npos);
    CPPUNIT_ASSERT(stdout.find("Title             The audio track") != string::npos);
    CPPUNIT_ASSERT(stdout.find("Encoder           likely some AAC encoder") != string::npos);
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
    size_t pos1 = stdout.find("Title             Big Buck Bunny - test 1");
    size_t pos2 = stdout.find("Title             Elephant Dream - test 2");
    size_t pos3 = stdout.find("Title             Elephant Dream - test 3");
    CPPUNIT_ASSERT(pos1 != string::npos);
    CPPUNIT_ASSERT(pos2 > pos1);
    CPPUNIT_ASSERT(pos3 > pos2);

    // set title and part number of 3 files at once
    const char *const args2[] = {"tageditor", "set", "target-level=30", "title=test1", "title=test2", "title=test3", "part+=1", "target-level=50", "title=MKV testfiles", "totalparts=3", "-f", mkvFile1.data(), mkvFile2.data(), mkvFile3.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT((pos1 = stdout.find("Matroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\n"
                               " Title             MKV testfiles\n"
                               " Year              2010\n"
                               " Comment           Matroska Validation File1, basic MPEG4.2 and MP3 with only SimpleBlock\n"
                               " Total parts       3\n"
                               "Matroska tag targeting \"level 30 'track, song, chapter'\"\n"
                               " Title             test1\n"
                               " Part              1")) != string::npos);
    CPPUNIT_ASSERT((pos2 = stdout.find("Matroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\n"
                               " Title             MKV testfiles\n"
                               " Year              2010\n"
                               " Comment           Matroska Validation File 2, 100,000 timecode scale, odd aspect ratio, and CRC-32. Codecs are AVC and AAC\n"
                               " Total parts       3\n"
                               "Matroska tag targeting \"level 30 'track, song, chapter'\"\n"
                               " Title             test2\n"
                               " Part              2")) > pos1);
    CPPUNIT_ASSERT((stdout.find("Matroska tag targeting \"level 50 'album, opera, concert, movie, episode'\"\n"
                               " Title             MKV testfiles\n"
                               " Year              2010\n"
                               " Comment           Matroska Validation File 3, header stripping on the video track and no SimpleBlock\n"
                               " Total parts       3\n"
                               "Matroska tag targeting \"level 30 'track, song, chapter'\"\n"
                               " Title             test3\n"
                               " Part              3")) > pos2);
}

/*!
 * \brief Tests tagging multiple values per field.
 */
void CliTests::testMultipleValuesPerField()
{
    // TODO (feature not implemented yet)
}

/*!
 * \brief Tests handling attachments.
 */
void CliTests::testHandlingAttachments()
{
    string stdout, stderr;
    const string mkvFile1(workingCopyPath("matroska_wave1/test1.mkv"));
    const string mkvFile2("path=" + testFilePath("matroska_wave1/test2.mkv"));

    // add attachment
    const char *const args2[] = {"tageditor", "set", "--add-attachment", "name=test2.mkv", "mime=video/x-matroska", "desc=Test attachment", mkvFile2.data(), "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args2, stdout, stderr));
    const char *const args1[] = {"tageditor", "info", "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    size_t pos1;
    CPPUNIT_ASSERT((pos1 = stdout.find("Attachments:")) != string::npos);
    CPPUNIT_ASSERT(stdout.find("Name                          test2.mkv") > pos1);
    CPPUNIT_ASSERT(stdout.find("MIME-type                     video/x-matroska") > pos1);
    CPPUNIT_ASSERT(stdout.find("Description                   Test attachment") > pos1);
    CPPUNIT_ASSERT(stdout.find("Size                          20.16 MiB (21142764 byte)") > pos1);

    // update attachment
    const char *const args3[] = {"tageditor", "set", "--update-attachment", "name=test2.mkv", "desc=Updated test attachment", "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args3, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT((pos1 = stdout.find("Attachments:")) != string::npos);
    CPPUNIT_ASSERT(stdout.find("Name                          test2.mkv") > pos1);
    CPPUNIT_ASSERT(stdout.find("MIME-type                     video/x-matroska") > pos1);
    CPPUNIT_ASSERT(stdout.find("Description                   Updated test attachment") > pos1);
    CPPUNIT_ASSERT(stdout.find("Size                          20.16 MiB (21142764 byte)") > pos1);

    // TODO: extract assigned attachment (feature not implemented yet)

    // remove assigned attachment
    const char *const args5[] = {"tageditor", "set", "--remove-attachment", "name=test2.mkv", "-f", mkvFile1.data(), nullptr};
    CPPUNIT_ASSERT_EQUAL(0, execApp(args5, stdout, stderr));
    CPPUNIT_ASSERT_EQUAL(0, execApp(args1, stdout, stderr));
    CPPUNIT_ASSERT(stdout.find("Attachments:") == string::npos);
    CPPUNIT_ASSERT(stdout.find("Name                          test2.mkv") == string::npos);
}

/*!
 * \brief Tests displaying general file info.
 */
void CliTests::testDisplayingInfo()
{
    // TODO (not very important)
}

/*!
 * \brief Tests extraction (used for cover or other binary fields).
 */
void CliTests::testExtraction()
{
    // TODO
}

/*!
 * \brief Tests reading and writing the document title.
 */
void CliTests::testReadingAndWritingDocumentTitle()
{
    // TODO
}
#endif
