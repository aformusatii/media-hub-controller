package spm;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import jssc.SerialPort;
import jssc.SerialPortEvent;
import jssc.SerialPortEventListener;
import jssc.SerialPortException;
import jssc.SerialPortList;

/**
 * @author Andrei
 *
 */
public class SerialPortMonitor implements SerialPortEventListener {

	private SerialPort serialPort;
	
	private String outputHtmlFilePath = "C:\\Temp\\IR\\LG_TV_ON_OFF_TMP.html";
	private HtmlWriter htmlWriter;
	
	private boolean isPacket = false;
	private boolean skipFirst = true;
	private int subPacketSize = 3;
	private byte[] subPacket = new byte[subPacketSize];
	private byte subPacketCount = 0;
	
	/**
	 * @param args
	 * @throws SerialPortException 
	 * @throws IOException 
	 */
	public static void main(String[] args) throws SerialPortException, IOException {
		new SerialPortMonitor().start();
	}
	
	public void start() throws SerialPortException, IOException {
		int portIndex = 0;
		boolean loop = true;
		
		String[] portNames = SerialPortList.getPortNames();

		for (String portName : portNames) {
			System.out.println("Port Name: " + portName);
		}
		
		htmlWriter = new HtmlWriter(outputHtmlFilePath);
		
		serialPort = new SerialPort(portNames[portIndex]);
		serialPort.openPort();		
		serialPort.setParams(SerialPort.BAUDRATE_19200, 
				             SerialPort.DATABITS_8, 
				             SerialPort.STOPBITS_1, 
				             SerialPort.PARITY_NONE);
		serialPort.addEventListener(this);
		
		while (loop) {
			String line = readLine("Subject: ");
			
			if ("exit".equalsIgnoreCase(line)) {
				break;
			}
			
			serialPort.writeByte((byte) 0x00);
		};
		
		serialPort.closePort();	
		htmlWriter.close();
	}

	
	@Override
	public synchronized void serialEvent(SerialPortEvent serialPortEvent) {
		//System.out.println(serialPortEvent.getEventType());
		
		try {
			byte[] data = serialPort.readBytes();
			
			for (byte b : data) {
				
				if (Delimiter.RESET.detected(b)) {
					System.out.println("RESET DETECTED");
					isPacket = false;
					continue;
				}
				
				if (Delimiter.START_PACKET.detected(b)) {
					System.out.println("START DETECTED");
					isPacket = true;
					skipFirst = true;
					subPacketCount = 0;
					htmlWriter.startRow();
					continue;
				}
				
				if (isPacket) {
					if (Delimiter.END_PACKET.matchNext(b, true)) {
						if (Delimiter.END_PACKET.lastDetected()) {
							System.out.println("END DETECTED");
							isPacket = false;
							htmlWriter.endRow();
							continue;						
						}
					} else {
						for (byte b2 : Delimiter.END_PACKET.getCheckBuffer()) {
								subPacket[subPacketCount++] = b2;
								
								if (subPacketCount == subPacket.length) {
									subPacketCount = 0;
									if (!skipFirst) {
										htmlWriter.append(subPacket);
									}
									skipFirst = false;
								}
						}
					}				
				}
				//System.out.print(String.format("%03d", (int) (b & 0xFF)) + " ");
			}
			
			//String str = new String(data);
			//System.out.println(str);
			
		} catch (SerialPortException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	private String readLine(String format, Object... args) throws IOException {
	    if (System.console() != null) {
	        return System.console().readLine(format, args);
	    }
	    
	    System.out.print(String.format(format, args));
	    BufferedReader reader = new BufferedReader(new InputStreamReader(
	            System.in));
	    return reader.readLine();
	}

}
