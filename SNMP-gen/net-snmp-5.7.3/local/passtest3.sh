#!/bin/bash
#This is a bash script, pay attention that you run in the right way. (/bin/bash).

#in the neighbour_sample file, the first line is the number of the neighbours.
#each line represents each member-entry.


# return the neighbours number (by redis)
# count number of sequence member instances
# PAY ATTENTION TO THE SPECIFIC PLACE (.2.16.1.1.*)

PLACE=".1.3.6.1.4.1.16215.1.24.1.4"
getId()
{
	id=$(tr -d "modem-" <<< $(hostname))
	echo $id
}


getNumOfMembers()
{
	id=$(getId)
        var=$(redis-cli -h $listen KEYS "*" | grep $id$PLACE.2.16.1.1.* -c)
	echo $var
}

getNumOfNeighbours()
{
 	id=$(getId)
        tmp=$(redis-cli -h $listen KEYS "*" | grep $id$PLACE.2.1.15.1.1.* -c)
	echo $tmp
}


listen=10.99.0.100
id=$(getId)
REQ="$2"                         # Requested OID
NUM_OF_NEIGHBOURS=$(getNumOfNeighbours)
NUM_OF_MEMBERS=$(getNumOfMembers)


#  Process SET requests by simply logging the assigned value
#      Note that such "assignments" are not persistent,
#      nor is the syntax or requested value validated
#  
if [ "$1" = "-s" ]; then
  REQ=$2
  VALUE=$4
  flag=0
  if [ $REQ = $PLACE.1.8 ]; then
      if [ $VALUE -ge 225000 -a $VALUE -le 511975 ]; then 
	  flag=1 
      fi 
  fi
  if [ $REQ = $PLACE.1.9 ]; then
      if [ $VALUE -ge 0 -a $VALUE -le 2 ]; then 
	  flag=1 
      fi 
  fi
  if [ $REQ = $PLACE.1.10 ]; then
      if [ $VALUE -ge 0 -a $VALUE -le 1 ]; then 
	  flag=1 
      fi 
  fi
  #set network-id
  if [ $REQ = $PLACE.1.11 ]; then
	  flag=1
	  redis-cli -h $listen SET $id$REQ $VALUE
	  ELBIT/emane-tutorial/snmpTDMA/setVal2EMANE.sh networkid $VALUE &
	  exit 0
  fi
  if [ $REQ = $PLACE.1.12 ]; then
	  flag=1
  fi
  if [ $REQ = $PLACE.1.13 ]; then
	  flag=1   
  fi
  if [ $REQ = $PLACE.1.14 ]; then
	  flag=1 
  fi


  if [ $flag -eq 1 ]; then
  tmp=$(redis-cli -h $listen SET $id$REQ $VALUE) 
  
  else
      echo "Invalid value" >> /tmp/check.txt
  fi
  exit 0
fi

#echo $NUM_OF_NEIGHBOURS >> /tmp/check.txt

#
#  GETNEXT requests ("-n") - determine next valid instance
#
# run over all optional places, and return the next place in the order.
if [ "$1" = "-n" ]; then
 RET=none
  case "$REQ" in
    $PLACE|		\
    $PLACE.0|		\
    $PLACE.0.*|		\
    $PLACE.1|		\
    $PLACE.1.1|		\
    $PLACE.1.1.*|	\
    $PLACE.1.2|		\
    $PLACE.1.2.*|	\
    $PLACE.1.3|		\
    $PLACE.1.3.*|	\
    $PLACE.1.4|		\
    $PLACE.1.4.*|	\
    $PLACE.1.5|		\
    $PLACE.1.5.*|	\
    $PLACE.1.6|		\
    $PLACE.1.6.*|	\
    $PLACE.1.7|		\
    $PLACE.1.7.*)     	RET=$PLACE.1.8 ;;
    $PLACE.1.8)       	RET=$PLACE.1.9 ;;
    $PLACE.1.9)       	RET=$PLACE.1.10 ;;
    $PLACE.1.10)      	RET=$PLACE.1.11 ;;
    $PLACE.1.11)      	RET=$PLACE.1.12 ;;
    $PLACE.1.12)      	RET=$PLACE.1.13 ;;
    $PLACE.1.13)      	RET=$PLACE.1.14 ;;
    $PLACE.1.14)      	RET=$PLACE.2.1 ;;
  esac
if [ $NUM_OF_NEIGHBOURS -eq 0 -a $NUM_OF_MEMBERS -eq 0 ]; then

    case "$REQ" in
    $PLACE.2.1)       	RET=$PLACE.3 ;;
    $PLACE.3)       	RET=$PLACE.4 ;;
    $PLACE.4)       	RET=$PLACE.5 ;;
    $PLACE.5)       	RET=$PLACE.6 ;;
    $PLACE.6)       	RET=$PLACE.7 ;;
    $PLACE.7)       	RET=$PLACE.8 ;;
    $PLACE.8)       	RET=$PLACE.9 ;;
    $PLACE.9)       	RET=$PLACE.10 ;;
    $PLACE.10)       	RET=$PLACE.11 ;;
    $PLACE.11)       	RET=$PLACE.12 ;;

    esac

else
  if [ $NUM_OF_NEIGHBOURS -ne 0 ];then

  for (( i=1; i<=$NUM_OF_NEIGHBOURS; i++ ))
  do

	if [ $REQ = $PLACE.2.1 ]; then
		RET=$PLACE.2.1.15.1.1.1

	elif [ $REQ = $PLACE.2.1.15.1.1.$i ]; then
		if [ $i = $NUM_OF_NEIGHBOURS ]; then
			RET=$PLACE.2.1.15.1.2.1
			break
		fi
		RET=$PLACE.2.1.15.1.1.$(($i+1))
		i=$NUM_OF_NEIGHBOURS

	elif [ $REQ = $PLACE.2.1.15.1.2.$i ]; then
		if [ $i = $NUM_OF_NEIGHBOURS ]; then
			RET=$PLACE.2.1.15.1.3.1
			break
		fi
		RET=$PLACE.2.1.15.1.2.$(($i+1))
		i=$NUM_OF_NEIGHBOURS

	elif [ $REQ = $PLACE.2.1.15.1.3.$i ]; then
		if [ $i = $NUM_OF_NEIGHBOURS ]; then
			RET=$PLACE.2.1.15.1.4.1
			break
		fi
		RET=$PLACE.2.1.15.1.3.$(($i+1))
		i=$NUM_OF_NEIGHBOURS

	elif [ $REQ = $PLACE.2.1.15.1.4.$i ]; then
		if [ $i = $NUM_OF_NEIGHBOURS ]; then
			RET=$PLACE.2.1.15.1.5.1
			break
		fi
		RET=$PLACE.2.1.15.1.4.$(($i+1))
		i=$NUM_OF_NEIGHBOURS

	elif [ $REQ = $PLACE.2.1.15.1.5.$i ]; then
		if [ $i = $NUM_OF_NEIGHBOURS ]; then
			RET=$PLACE.2.1.15.1.6.1
			break
		fi
		RET=$PLACE.2.1.15.1.5.$(($i+1))
		i=$NUM_OF_NEIGHBOURS

	elif [ $REQ = $PLACE.2.1.15.1.6.$i ]; then
		if [ $i = $NUM_OF_NEIGHBOURS ]; then
			break
		fi
		RET=$PLACE.2.1.15.1.6.$(($i+1))
	fi
  done
	if [ $NUM_OF_MEMBERS -ne 0 ];then
	# MembersEntry sequence
	 for (( i=1; i<=$NUM_OF_MEMBERS; i++ ))
	  do
	
		if [ $REQ = $PLACE.2.1.15.1.6.$NUM_OF_NEIGHBOURS ]; then
			RET=$PLACE.2.16.1.1.1

		elif [ $REQ = $PLACE.2.16.1.1.$i ]; then
			if [ $i = $NUM_OF_MEMBERS ]; then
				RET=$PLACE.2.16.1.2.1
				break
			fi
			RET=$PLACE.2.16.1.1.$(($i+1))
			i=$NUM_OF_MEMBERS

		elif [ $REQ = $PLACE.2.16.1.2.$i ]; then
			if [ $i = $NUM_OF_MEMBERS ]; then
				RET=$PLACE.2.16.1.3.1
				break
			fi
			RET=$PLACE.2.16.1.2.$(($i+1))
			i=$NUM_OF_MEMBERS

		elif [ $REQ = $PLACE.2.16.1.3.$i ]; then
			if [ $i = $NUM_OF_MEMBERS ]; then
				RET=$PLACE.2.16.1.4.1
				break
			fi
			RET=$PLACE.2.16.1.3.$(($i+1))
			i=$NUM_OF_MEMBERS

		elif [ $REQ = $PLACE.2.16.1.4.$i ]; then
			if [ $i = $NUM_OF_MEMBERS ]; then
				break
			fi
			RET=$PLACE.2.16.1.4.$(($i+1))
		fi
	  done
	  case "$REQ" in
	      $PLACE.2.16.1.4.$NUM_OF_MEMBERS)       	RET=$PLACE.3 ;;
	      $PLACE.3)       	RET=$PLACE.4 ;;
	      $PLACE.4)       	RET=$PLACE.5 ;;
	      $PLACE.5)       	RET=$PLACE.6 ;;
	      $PLACE.6)       	RET=$PLACE.7 ;;
	      $PLACE.7)       	RET=$PLACE.8 ;;
	      $PLACE.8)       	RET=$PLACE.9 ;;
	      $PLACE.9)       	RET=$PLACE.10 ;;
	      $PLACE.10)       	RET=$PLACE.11 ;;
	      $PLACE.11)       	RET=$PLACE.12 ;;
	  esac

	#if NUM_OF_NEIGHBOURS not equal zero but NUM_OF_MEMBERS = 0
	else
	case "$REQ" in
	    $PLACE.2.1.15.1.6.$NUM_OF_NEIGHBOURS)       	RET=$PLACE.3 ;;
	    $PLACE.3)       	RET=$PLACE.4 ;;
	    $PLACE.4)       	RET=$PLACE.5 ;;
	    $PLACE.5)       	RET=$PLACE.6 ;;
	    $PLACE.6)       	RET=$PLACE.7 ;;
	    $PLACE.7)       	RET=$PLACE.8 ;;
	    $PLACE.8)       	RET=$PLACE.9 ;;
	    $PLACE.9)       	RET=$PLACE.10 ;;
	    $PLACE.10)       	RET=$PLACE.11 ;;
	    $PLACE.11)       	RET=$PLACE.12 ;;
	esac
	
	fi
	
#if NUM_OF_NEIGHBOURS = 0 then NUM_OF_MEMBERS not equal zero
else
# MembersEntry sequence
 for (( i=1; i<=$NUM_OF_MEMBERS; i++ ))
  do
	
	if [ $REQ = $PLACE.2.1 ]; then
		RET=$PLACE.2.16.1.1.1

	elif [ $REQ = $PLACE.2.16.1.1.$i ]; then
		if [ $i = $NUM_OF_MEMBERS ]; then
			RET=$PLACE.2.16.1.2.1
			break
		fi
		RET=$PLACE.2.16.1.1.$(($i+1))
		i=$NUM_OF_MEMBERS

	elif [ $REQ = $PLACE.2.16.1.2.$i ]; then
		if [ $i = $NUM_OF_MEMBERS ]; then
			RET=$PLACE.2.16.1.3.1
			break
		fi
		RET=$PLACE.2.16.1.2.$(($i+1))
		i=$NUM_OF_MEMBERS

	elif [ $REQ = $PLACE.2.16.1.3.$i ]; then
		if [ $i = $NUM_OF_MEMBERS ]; then
			RET=$PLACE.2.16.1.4.1
			break
		fi
		RET=$PLACE.2.16.1.3.$(($i+1))
		i=$NUM_OF_MEMBERS

	elif [ $REQ = $PLACE.2.16.1.4.$i ]; then
		if [ $i = $NUM_OF_MEMBERS ]; then
			break
		fi
		RET=$PLACE.2.16.1.4.$(($i+1))
	fi
  done
case "$REQ" in
    $PLACE.2.16.1.4.$NUM_OF_MEMBERS)       	RET=$PLACE.3 ;;
    $PLACE.3)       	RET=$PLACE.4 ;;
    $PLACE.4)       	RET=$PLACE.5 ;;
    $PLACE.5)       	RET=$PLACE.6 ;;
    $PLACE.6)       	RET=$PLACE.7 ;;
    $PLACE.7)       	RET=$PLACE.8 ;;
    $PLACE.8)       	RET=$PLACE.9 ;;
    $PLACE.9)       	RET=$PLACE.10 ;;
    $PLACE.10)       	RET=$PLACE.11 ;;
    $PLACE.11)       	RET=$PLACE.12 ;;
esac

fi
fi

 if [ $RET = none ]; then
 	exit 0
 fi
fi


#
#  GET requests - check for valid instance
#
# check if the REQ is an exist PLACE

if [ "$1" = "-g" ]; then 

 isSeqence=$0

#check if seqence
 for (( i=1; i<=$NUM_OF_NEIGHBOURS; i++ ))
  do
	if [ $REQ = $PLACE.2.1.15.1.1.$i ] ||
	   [ $REQ = $PLACE.2.1.15.1.2.$i ] ||
	   [ $REQ = $PLACE.2.1.15.1.3.$i ] ||
	   [ $REQ = $PLACE.2.1.15.1.4.$i ] ||
	   [ $REQ = $PLACE.2.1.15.1.5.$i ] ||
	   [ $REQ = $PLACE.2.1.15.1.6.$i ]; then
		RET=$REQ
		isSeqence=$1
	fi
  done


 for (( i=1; i<=$NUM_OF_MEMBERS; i++ ))
  do
	if [ $REQ = $PLACE.2.16.1.1.$i ] ||
	   [ $REQ = $PLACE.2.16.1.2.$i ] ||
	   [ $REQ = $PLACE.2.16.1.3.$i ] ||
	   [ $REQ = $PLACE.2.16.1.4.$i ]; then
		RET=$REQ
		isSeqence=$1
	fi
  done
#if not seqence
if [ "$isSeqence" = "$0" ]; then 
  case "$REQ" in
    $PLACE.1.8|		\
    $PLACE.1.9|		\
    $PLACE.1.10|	\
    $PLACE.1.11|	\
    $PLACE.1.12|	\
    $PLACE.1.13|	\
    $PLACE.1.14|	\
    $PLACE.2.1|		\
    $PLACE.3|		\
    $PLACE.4|		\
    $PLACE.5|		\
    $PLACE.6|		\
    $PLACE.7|		\
    $PLACE.8|		\
    $PLACE.9|		\
    $PLACE.10|		\
    $PLACE.11|		\
    $PLACE.12)   RET=$REQ ;;
    *)         	    exit 0 ;;
  esac
 fi
fi



echo "$RET"
isSeqence=$0

#print seqence
for (( i=1; i<=$NUM_OF_NEIGHBOURS; i++ ))
  do
	if [ $RET = $PLACE.2.1.15.1.1.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.1.15.1.1.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.1.15.1.2.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.1.15.1.2.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.1.15.1.3.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.1.15.1.3.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.1.15.1.4.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.1.15.1.4.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.1.15.1.5.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.1.15.1.5.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.1.15.1.6.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.1.15.1.6.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	fi
  done

for (( i=1; i<=$NUM_OF_MEMBERS; i++ ))
  do
	if [ $RET = $PLACE.2.16.1.1.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.16.1.1.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.16.1.2.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.16.1.2.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.16.1.3.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.16.1.3.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	elif [ $RET = $PLACE.2.16.1.4.$i ]; then
		val=$(redis-cli -h $listen get $id$PLACE.2.16.1.4.$i)
		echo "integer";	echo $val;	exit 0 ;
		isSeqence=$1
	fi
  done
#print non seqence
if [ "$isSeqence" = "$0" ]; then 
 case "$RET" in
   $PLACE.1.8)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.1.8);	exit 0 ;;
   $PLACE.1.9)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.1.9);	exit 0 ;;
   $PLACE.1.10)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.1.10);	exit 0 ;;
   $PLACE.1.11)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.1.11);	exit 0 ;;
   $PLACE.1.12)		echo "integer"; echo $(redis-cli -h $listen get $id$PLACE.1.12);	exit 0 ;;
   $PLACE.1.13)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.1.13);	exit 0 ;;
   $PLACE.1.14)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.1.14);	exit 0 ;;
   $PLACE.2.1)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.2.1);	exit 0 ;;
   $PLACE.3)		echo "string";	echo $(redis-cli -h $listen get $id$PLACE.3);	exit 0 ;;
   $PLACE.4)		echo "string";	echo $(redis-cli -h $listen get $id$PLACE.4);	exit 0 ;;
   $PLACE.5)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.5);	exit 0 ;;
   $PLACE.6)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.6);	exit 0 ;;
   $PLACE.7)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.7);	exit 0 ;;
   $PLACE.8)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.8);	exit 0 ;;
   $PLACE.9)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.9);	exit 0 ;;
   $PLACE.10)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.10);	exit 0 ;;
   $PLACE.11)		echo "integer";	echo $(redis-cli -h $listen get $id$PLACE.11);	exit 0 ;;
   $PLACE.12)		echo "string";	echo $(redis-cli -h $listen get $id$PLACE.12);	exit 0 ;;
   *)            	echo "string";  echo "ack... $RET $REQ";   exit 0 ;;  # Should not happen
 esac
fi

