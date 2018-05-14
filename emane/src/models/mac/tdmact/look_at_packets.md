  # Parse packets in iov format
  
  ## get them
    auto vv = pkt.getVectorIO();
    int priority = pkt.getPacketInfo().getPriority();
    std::uint8_t u8Queue{priorityToQueue(pkt.getPacketInfo().getPriority())};



  ## look at their content

  if (0) {
    auto vv = pkt.getVectorIO();
    for (x: vv) {
      LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "TC_MACI %03hu TDMA::BaseModel::%s downstream %d %.*s",
                          id_,
                          __func__, (int) x.iov_len, (int) x.iov_len, (char *) x.iov_base);
      int fd;
      fd=open("/tmp/TCmsg.bin",O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
      pwritev(fd, &x, 1, 0);
      close(fd);
    }
  }


  ## parse type

    if (1)
  {
    char const *arp_str = "ARP";
    char const *ip_str = "IP_?";
    char const *udp_str = "IP_UDP_?";
    char const *olsr_str = "IP_UDP_OLSR_?";
    char const *hello_str = "IP_UDP_OLSR_HELLO";
    char const *icmp_str = "IP_ICMP";
    char const *c = "unknown";
    
    unsigned char *packet;
    packet = (unsigned char *) vv[0].iov_base;

    if ( packet[12] == 0x08 && packet[13] == 0x06 ) { // ARP
      c = arp_str;
    }
    else if (packet[12] == 0x08 && packet[13] == 0x00) { // IP
      c = ip_str;
      if (packet[23] == 0x11){ // UDP
        c = udp_str;
        if (packet[34] == 0x02 && packet[35] == 0xBA ){ // OLSR (checks source port in UDP)
          c = olsr_str;
          if (packet[46] == 0xc9) { // HELLO
            c = hello_str;
          }
        }
      }
      else if (packet[23] == 0x01){ // ICMP
        c = icmp_str;
      }
    }
    LOGGER_STANDARD_LOGGING(pPlatformService_->logService(),
                          DEBUG_LEVEL,
                          "TC_MACI %03hu TDMA::BaseModel::%s enqueued %s len %d  pr %d queue %d drop %d",
                          id_,
                          __func__, c, (int) vv[0].iov_len, priority, u8Queue, (int) packetsDropped);
  }
