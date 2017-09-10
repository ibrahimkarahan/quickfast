# Command file to set QuickFAST environment
# QuickFAST depends on MPC V 3.6 or later. (http://www.ociweb.com/products/mpc)
# QuickFAST depends on BOOST V 1.36.0 or later. (http://www.boost.org/)
# QuickFAST depends on Xerces V3.0 or later. (http://xerces.apache.org/xerces-c/)
# Customize this file by setting variables to suit your environment
SOURCE="${BASH_SOURCE[0]}"
SOURCE_DIR=`dirname $SOURCE`
export QUICKFAST_ROOT=`readlink -f $SOURCE_DIR`

if test "$MPC_ROOT" = ""
then
  export MPC_ROOT=
fi

if test "$BOOST_ROOT" = ""
then
  export BOOST_ROOT=/usr
fi

if test "$BOOST_ROOT_LIB" = ""
then
  export BOOST_ROOT_LIB=$BOOST_ROOT/lib
fi

if test "$BOOST_VERSION" = ""
then
  export BOOST_VERSION=
fi

if test "$XERCES_ROOT" = ""
then
  export XERCES_ROOT=/usr
fi

if test "$XERCES_LIBPATH" = ""
then
  export XERCES_LIBPATH=$XERCES_ROOT/lib
fi

if test "$XERCES_LIBNAME" = ""
then
  export XERCES_LIBNAME=xercesc
fi

if test "$XERCES_INCLUDE" = ""
then
  export XERCES_INCLUDE=$XERCES_ROOT/include
fi

export PATH=$QUICKFAST_ROOT/bin:$MPC_ROOT:$PATH
export LD_LIBRARY_PATH=$XERCESLIB:$QUICKFAST_ROOT/lib:$BOOST_ROOT_LIB:$LD_LIBRARY_PATH

