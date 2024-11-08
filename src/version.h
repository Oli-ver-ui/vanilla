static char version[] = "Vanilla version 3.4.25";

/**
 ** Version log
 **
 ** Version 3.4.25 - Fri Jul  1 09:44:28 MST 2005
 **     vanilla may declare a false match when
 **     two records from two different fragments
 **     of the same table land at the same address
 **     and the first record matches the first
 **     selection criterion and the second record
 **     matches the second selection criterion.
 **
 ** Version 3.4.24 - Tue May 25 13:24:43 MST 2004
 **     When opening a fragment with detached label
 **     with explicit filename in ^TABLE, vanilla
 **     is case-sensitive. I've added an crude
 **     method to match the case with the ".lbl"
 **     file. This should work for most of the cases.
 **
 **     If the START and STOP key-values belong to a
 **     character/string key-field, any enclosing 
 **     double-quotes are removed from these
 **     key-values.
 **
 **     - Wed Apr 21 15:17:21 MST 2004
 **     When table name is enclosed in quotes within
 **     table fragments, vanilla choked on the 
 **     "tbl.fld_name" construction in the 
 **     "-fields" and "-select".
 **
 **     Fixed unnecessary LoadTable for blank lines
 **     in the DATASET.
 **
 ** Version 3.4.23 - Thu Mar  6 12:01:51 MST 2003
 **     PDS label parsing code overwrote out of
 **     bounds memory when a value with inline
 **     units did not have space inbetween them.
 **     For example:
 **           INSTRUMENT_FOV = 20<MRAD>
 **
 **     Added compile and link options in makefile
 **     to generate static binary on HPUX platform.
 **
 ** Version 3.4.22 - Fri Jun 14 15:03:44 MST 2002
 **     Fake table name is new extracted from the
 **     first file specified with the "-files"
 **     option.
 **
 ** Version 3.4.21 - Fri Jun 14 11:37:24 MST 2002
 **     Converted DOS/Windows back-slash paths to
 **     Unix style forward-slash for the 
 **     files specified for the "-files" option.
 **
 ** Version 3.4.20 - Fri Jun 14 09:38:09 MST 2002
 **     Option checks modified so that the user
 **     is not able to specify conflicting options on
 **     the command line.
 **
 ** Version 3.4.19 - Fri Jun 14 08:51:25 MST 2002
 **     Logging code fixed.
 **
 ** Version 3.4.18 - Thu Jun 13 13:19:04 MST 2002
 **     Added a new command-line option, "-files"
 **     which allows vanilla to interpret a list of
 **     files specified on the command-line as a 
 **     table.
 **
 **     Logging code still needs fixing and the 
 **     indexing code needs checks put in to avoid
 **     users use indexing options incorrectly.
 **
 ** Version 3.4.17 - Tue Jun 11 17:51:31 MST 2002
 **     vanilla can now correctly read TABLE data from 
 **     a multi-object PDS label. Restrictions of a single 
 **     TABLE object still remain enforced though.
 **
 ** Version 3.4.16 - Thu Jun  6 18:00:00 MST 2002
 **      Added restricted support for unkeyed tables.
 **      Only one such table per DATASET is allowed.
 **      Necessary checks to disable options that don't
 **      or will not work with such tables have yet to
 **      be put in.
 **
 ** Version 3.4.15 - Wed Jun  5 19:15:14 MST 2002
 **      Added support to read data files with 
 **      detached labels.
 **
 ** Version 3.4.14 - Tue Jun  4 16:55:57 MST 2002
 **      Absolute paths with drive letters were not
 **      working correctly in DOS/Windows DATASETs.
 **
 ** Version 3.4.13 - Mon Jun  3 21:43:45 MST 2002
 **      Spotted during addition of reading detached
 **      labels functionality: EquivalantData had a
 **      logic error which prevented vanilla from 
 **      working correctly with tables with STRING
 **      based keys.
 **
 ** Version 3.4.12 - Fri Apr 12 17:23:14 MST 2002
 **      Reported by Jeff R. Johnson USGS Flagstaff, AZ
 **      Only first 148 channels of emissivity contain
 **      data, channels 149-286 contain zeroes.
 **
 ** Version 3.4.11 - Fri Jan 11 18:41:01 MST 2002
 **      vanilla failed on a non-existent directory
 **      in the dataset.
 **
 ** Version 3.4.10 - Thu Dec 20 14:08:07 MST 2001
 **      MAXHOSTNAME in logging.c was not available
 **      on all systems. Replaced by a fixed size of
 **      1024 bytes.
 **
 **                - Wed Dec 19 22:41:28 MST 2001
 **      Distribution makefile for logging turned on.
 **      Fixed make dist to put everything into a directory now.
 **
 **                - Wed Dec 19 18:25:47 MST 2001
 **      Table fragment name sorting is case insensitive now.
 **      Fixed broken compare function in qsort().
 **
 **                - Mon Dec 17 22:03:52 MST 2001
 **      Changed some slash-slash comments to slash-star
 **      comments.
 **      Added a section for AIX compilation in Makefile
 **      Fixed makefile.win
 **
 **                - Wed Dec 12 16:08:25 MST 2001
 **      Renamed the following files to maintain
 **      PDS 8.3 file naming standard:
 **          vindexbits.[ch] renamed to vidxbits.[ch]
 **          vindexuse.c renamed to vidxuse.c
 **          vindex.[ch] renamed to vidx.[ch]
 **          rough_count.[ch] renamed to rough_ct.[ch]
 **
 ** Version 3.4.9 - Thu Nov 29 16:55:01 MST 2001
 **      If a "-select" field is specified multiple
 **      times. The multiple instances are assumed
 **      to be ORing to each other's selection.
 **      Thus "-select 'detector 1 3 detector 6 6'"
 **      will select detectors 1-3 and 6.
 **
 **      Added code for indexing fixed-length array
 **      fields.
 **
 ** Version 3.4.8 - Wed Nov 28 13:42:20 MST 2001
 **      Date on Version 3.4.7 below is incorrect.
 **      ctime_r replaced by ctime for compatibility reasons.
 **
 ** Version 3.4.7 - Thu Nov 28 13:40:00 MST 2001
 **      Merged branch vanilla-3-3-13-key-alias-fix at
 **      vanilla-3-3-13-4 with vanilla-3-4-6
 **
 ** Version 3.4.6 - Wed Sep 13 09:03:12 MST 2000
 **      Added rough indexing statistics through the 
 **      use of two additional command-line options:
 **
 **        -count  
 **           returns minimum record based on indexing
 **           for the specified select
 **
 **        -idxuse 
 **           returns index statistics for specified select
 **
 ** Version 3.4.5 - Fri Sep  1 10:07:40 MST 2000
 **      Added type-4 indices.
 **      Index update fixes.
 **      Added bit-fields based index searching during
 **      query processing.
 **      ** This version has both qsort()-based and
 **      ** bits-based index searching, switchable
 **      ** thru Makefile
 **
 ** Version 3.4.4 - Wed Aug  9 15:09:39 MST 2000
 **      Added userid, username, and hostname in logged data.
 **      Moved the counter increment "ct[i]++" in vindexuse.c 
 **      before the line where "i" is incremented.
 **
 ** Version 3.4.3 - Thu Aug  3 18:52:00 MST 2000
 **      Source maintenance update.
 **         Renamed STATS to LOGGING.
 **         Rename hstats.c to logging.c
 ** 
 **      Every Usage invokation was being logged, because
 **      usage() was being called from exit(). It has been
 **      changed to a return() instead.
 **
 ** Version 3.4.2 - Thu Aug  3 11:11:35 MST 2000
 **      Added logging to /tes/lib/vanilla.log at termination.
 **      It is controlled at compile time by setting the STATS
 **      define (macro).
 **
 ** Version 3.4.1 : Tue Jul 25 18:19:07 MST 2000
 **     Added PC_REAL, removed some #if 0 code
 **
 ** Version 3.4 - Tue Jul 25 17:17:09 MST 2000
 **     Added preliminary code to create/use indices for
 **     searching.
 **
 **     Fragment record access mechanism has also been changed
 **     from a simple pointer to a record-handle structure.
 **
 ** Version 3.3.13.4 - Tue May 22 15:54:39 MST 2001
 **     Binary output for fake-fields had text delimiters in
 **     them. This is due to the result of multiple things:
 **
 **     i)  The output-frame-size is duplicated in all output
 **         fields except for the fields added to the output
 **         fields' list because the fake-field needs them.
 **         The output frame-size should go into a global
 **         variable for maintaining data consistency.
 **         However, the functions using this global variable
 **         cannot be easily moved to other vanilla tools
 **         then.
 **
 **     ii) While outputting data, dependencies of the fake
 **         fields were being skipped. It is the correct thing
 **         to do. However, this skipping had a serious side
 **         effect when combined with (i). The frame-size 
 **         obtained after skipping the dependencies came
 **         out to be for text-output.
 **        
 **
 ** Version 3.3.13.3 - Mon May 21 17:41:51 MST 2001
 **     Windows 95 stat() fails if the path is suffixed
 **     with a "/" (e.g. stat on "/vanilla/data/" will
 **     yield -1). Windows NT does not complain however.
 **
 ** Version 3.3.13.2 - Mon May 14 17:00:14 MST 2001
 **     Enabled binary output from fake-fields. The code was
 **     already there. A test that did not allow it to run
 **     has been removed.
 **
 **     Changes from Version 3.3.14 - Tue Jul 18 14:12:17 MST 2000
 **     (which was never checked in) have also been rolled into this
 **     version. Description of these changes follow:
 **		   Forgot to handle LSB VAR byte count. We now assume that
 **        if the data field is LSB, so is the byte counter.
 **
 **
 ** Version 3.3.13.1 - Mon Jul 31 14:00:59 MST 2000
 **      Added:
 **         key-name / key-alias <-> key-name / key-alias 
 **      matching. This change will help in a join-query
 **      when the tables involved include both, PDS labels
 **      and internal labels.
 **
 **      If we have the following two keys in "obs" and "rad"
 **      tables respectively:
 **         obs.SPACE_CRAFT_CLOCK_TIME with alias obs.sclk_time
 **         rad.sclk_time with no alias
 **
 **      The new code will take into account the field 
 **      thus resulting in "sclk_time" being the common key
 **      between "obs" and "rad" tables.
 **
 ** Version 3.3.13 - Mon Jul 17 17:48:50 MST 2000
 **		Added support for LSB_INTEGER and friends
 **			
 ** Version 3.3.11 - Wed Jul  5 17:39:01 MST 2000
 **      Modified Win95 version to use MS Visual C++ memory
 **      mapping facilities.
 **      Removed some of the unused variables from a bunch
 **      of source files.
 **
 ** Version 3.3.10 - Fri May 26 08:00:42 MST 2000
 **		Changed return type for LoadTable from void to int
 **		Check return type of LoadTable in LoadDataset
 **		Fixed core dump crashes when LoadLabel returns NULL
 **		
 ** 
 ** Version 3.3.9 - Wed May 24 19:21:19 MST 2000
 **      Fixed indices for IRTM calculation
 **
 ** Version 3.3.8 - Tue Apr 25 13:06:03 MST 2000
 **      Upgraded header.c to recognize SUN_* and MAC_* data formats
 **
 ** Version 3.3.7 - Thu Feb 10 15:11:03 MST 2000
 **      Bug fix in V3.3.6 was not working properly, fixed.
 **
 ** Version 3.3.6 - Wed Feb  9 11:06:00 MST 2000
 **      Bug in searching the appropriate fragment in a table
 **      given a key. buffs.c was not checking the limit on
 **      the number of fragments available, thus leading to a 
 **      core dump.
 **
 ** Version 3.3.5 - Mon Feb  7 20:54:50 MST 2000
 **      Minor bug in print_atom for vardata.  It was using the 
 **      format of the "field" instead of the "varfield".
 **
 **      output.c/ConvertByteCount() was not enforcing the correct
 **      MSB/LSB conversion. Fixed.
 **
 ** Version 3.3.4 - Thu Jan  6 13:56:35 MST 2000
 **      -select on array[i] selected on array[0]
 **      instead of array[i]. Fixed.
 **
 ** Version 3.3.3 - Mon Dec 20 16:59:13 MST 1999
 **      Fixed global variable assignment, "ofptr = stdout"
 **      Fixed compile error in WINDOWS compile, by declaring
 **      PROT_WRITE and MAP_PRIVATE as dummy defines
 **
 ** Version 3.3.2 - Mon Dec 20 14:24:51 MST 1999
 **      Fixed invalid index dereference in ff_t20.c
 **
 ** Version 3.3.1 - Mon Dec 20 11:59:47 MST 1999
 **      Buffer Management Fix:
 **      While skipping over fragments that do not contain the
 **      required sclk_time, the "end" pointer was not being
 **      set properly, hence, some of the records were being
 **      missed from being processed.
 **
 ** Version 3.3 - Tue Dec  7 11:42:12 MST 1999
 **      Added thermal inertia fake fields
 **      a) fake field "t20" as in IDL-fudge3.pro
 **      b) fake field "irtm[1-5]"
 **
 ** Version 3.2.1 - Tue Nov 23 14:21:52 MST 1999
 **      Fixed cord dump on selecting fake field "emmissivity"
 **      after attaching the new output routines in V3.2.
 **
 ** Version 3.2 - Fri Nov 19 17:01:54 MST 1999
 **      Added binary output capability with flag -B
 **
 ** Version 3.1 - Fri Nov 19 11:12:13 MST 1999
 **      Fixed the field heading for byte offset type fields from
 **      "field[0]" to "field" only
 **
 ** Version 3.0 -  Fri Nov 12 11:56:22 MST 1999
 **      Broken LoadLabel() into LoadLabel() and LoadFmt() routines
 **      Code maintenance fixes only
 **
 ** Version 2.8 -  Fri Sep  3 13:42:37 MST 1999
 **      Found that PDS style START_PRIMARY_KEY and STOP_PRIMARY_KEY
 **      aren't being read properly.  Fixed
 **
 ** Version 2.7 -  Tue Aug 10 12:13:14 MST 1999
 **      Added fake fields (emissivity)
 **
 ** Version 2.6 -  Mon Jul 12 15:16:17 MST 1999
 **      Merged source trees, fixed a validation bug in search.c
 **
 ** Version 2.4 -  Sun May  2 18:02:06 MST 1999
 **      patched some path problems under DOS
 ** 
 ** Version 2.3 - 
 **      Rename DATASET to dataset.lst
 **
 ** Version 2.1 -  Tue Feb  9 21:01:22 MST 1999
 **      Second try for PDS version
 **
 ** Version 2.0 -  Thu Dec 17 18:15:20 MST 1998
 **      Cleanup and version increment for release.
 **/
