#!/bin/sh

#
# Shell script to analyze the xpcom log file
#
# Usage: xpcom-log-analize.sh [<logfile>] > xpcom-log.html
#
# You can generate the xpcom log file using:
#    setenv NSPR_LOG_MODULES nsComponentManager:5
#    setenv NSPR_LOG_FILE xpcom.log
#    ./mozilla
#    <quit>
#

if [ $# -gt 0 ]
then
    logfile=$1
else
    logfile=xpcom.log
fi

# make sure we got the logfile
if [ ! -f $logfile ]
then
    echo "Error: Cannot find xpcom logfile $logfile"
    exit -1
fi

echo "<HTML>"
echo "<HEAD>"
echo "<TITLE>XPCOM Log analysis dated:"
date
echo "</TITLE></HEAD>"
echo "<H1><center>"
echo "XPCOM Log analysis dated:"
date
echo "</center></H1>"
# ========================================================================
# Performance analysis
# ========================================================================
echo "<H2>Performance Analysis</H2>"

# Number of dlls loaded
n=`grep -i ': loading' xpcom.log | wc -l`
echo "<H3>Dlls Loaded : $n</H3>"
echo "<blockquote><pre>"
grep ': loading' $logfile
echo "</pre></blockquote>"

# Objects created with a histogram
n=`grep -i 'createinstance.*succeeded' xpcom.log | wc -l`
echo "<H3>Objects created : $n</H3>"
echo "<blockquote><pre>"
grep -i 'createinstance.*succeeded' xpcom.log | sort | uniq -c | sort -nr
echo "</pre></blockquote>"

# Number of RegisterFactory
n=`grep 'RegisterFactory' $logfile | wc -l`
echo "<H3>Factory registrations : $n</H3>"
echo "<blockquote><pre>"
grep 'RegisterFactory' $logfile
echo "</pre></blockquote>"

# Number of RegisterComponentCommon.
# XXX Order them by dllname
n=`grep 'RegisterComponentCommon({' $logfile | wc -l`
echo "<H3>Component Registrations : $n</H3>"
echo "<blockquote><pre>"
grep 'RegisterComponentCommon({' $logfile
echo "</pre></blockquote>"

# Number of succeeded calls to ProgIDToClassID() with histogram
n=`grep -i 'progidtoclassid.*}' $logfile | wc -l`
echo "<H3>Count of succeeded ProgIDToClassID() : $n</H3>"
echo "<blockquote><pre>"
grep -i 'progidtoclassid.*}' $logfile | sort | uniq -c | sort -nr
echo "</pre></blockquote>"


# ========================================================================
# Error analysis
# ========================================================================
echo "<H2>Error Analysis</H2>"

# CreateInstance() FAILED
n=`grep 'CreateInstance.*FAILED' $logfile | wc -l`
echo "<H3>Failed CreateInstance() : $n</H3>"
echo "<blockquote><pre>"
grep 'CreateInstance.*FAILED' $logfile
echo "</pre></blockquote>"

# ProgIDToClassID() FAILED with a histogram
n=`grep 'ProgIDToClassID.*FAILED' $logfile | wc -l`
echo "<H3>Failed ProgIDToClassID() : $n</H3>"
echo "<blockquote><pre>"
grep 'ProgIDToClassID.*FAILED' $logfile | sort | uniq -c | sort -nr
echo "</pre></blockquote>"

#
# Cleanup
echo "</HTML>"