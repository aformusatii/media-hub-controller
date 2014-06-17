package spm;

import java.util.Arrays;

public enum Delimiter {
	RESET("INIT AFTER RESET"),
	START_PACKET("START PACKET"),
	END_PACKET("END PACKET");
	
	private int index = 0;
	private byte[] markerBytes;
	private int bIndex = 0;
	private byte[] checkBuffer;
	
	private Delimiter(String value) {
		this.markerBytes = value.getBytes();
		this.checkBuffer = new byte[this.markerBytes.length];
	}
	
	public boolean detected(byte b) {
		return matchNext(b, false) && lastDetected();
	}
	
	public boolean lastDetected() {
		if (index == markerBytes.length) {
			reset();
			return true;			
		} else {
			return false;	
		}
	}
	
	public boolean matchNext(byte b, boolean capture) {
		if (capture) {
			checkBuffer[bIndex++] = b;
		}
		
		if (markerBytes[index++] == b) {
			return true;
		} else {
			index = 0;
			return false;
		}
	}
	
	public byte[] getCheckBuffer() {
		byte[] buffer = Arrays.copyOf(checkBuffer, bIndex);
		bIndex = 0;
		return buffer;
	}
	
	private void reset() {
		index = 0;
		bIndex = 0;		
	}
	
}
