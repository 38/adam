class DatagramSocket{
	DatagramSocket(int port){
		__Report__.report_1("Trying to open a UDP socket", port);
	}
	public void send(java.net.DatagramPacket pkt)
	{
		__Report__.report_1("Sending data via UDP socket", pkt);
	}
	public void receive(java.net.DatagramPacket pkt)
	{
		__Tag__._16(pkt); //10 means network
	}
}
