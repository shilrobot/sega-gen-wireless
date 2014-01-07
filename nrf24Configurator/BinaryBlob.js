function BinaryBlob(size) {
	console.assert(size > 0);
	this.size = size;
	this.bits = [];
	for (var i=0; i<size; ++i) {
		this.bits[i] = 0;
	}
}

BinaryBlob.fromInt = function(x, bits) {
	var result = new BinaryBlob(bits);
	for (var i=0; i<bits; ++i) {
		if (x & (1<<i)) {
			result.bits[i] = 1;
		}
	}		
	return result;
}

BinaryBlob.fromBinaryString = function(str) {
	var size = 0;
	var bits = [];
	for (var i=0; i<str.length; ++i) {
		var c = str.charAt(i);
		if (c == '1') {
			bits[size] = 1;
			size++;
		} else if (c == '0') {
			bits[size] = 0;
			size++;
		} else {
			// skip
		}
	}		
	bits.reverse();
	var result = new BinaryBlob(size);
	result.bits = bits;
	return result;
}

BinaryBlob.fromHexString = function(str, size) {
	if (size === undefined) {
		size = str.length*4;
	}

	if (str.length*4 > size) {
		throw "String too big";
	}

	var result = new BinaryBlob(size);

	for (var i=str.length-1, j=0; i >=0; --i,j+=4) {
		var c = str.charAt(i);
		if ("0123456789ABCDEFabcdef".indexOf(c) < 0) {
			throw "Bad hex digit";
		}
		var nibble = parseInt(c,16);
		if (j < size) {
			result.bits[j] = (nibble & 1)?1:0;
			if (j+1 < size) {
				result.bits[j+1] = (nibble & 2)?1:0;
			}
			if (j+2 < size) {
				result.bits[j+2] = (nibble & 4)?1:0;
			}
			if (j+3 < size) {
				result.bits[j+3] = (nibble & 8)?1:0;
			}
		}
	}

	return result;
}

BinaryBlob.prototype.copy = function() {
	var result = new BinaryBlob(this.size);
	result.bits = this.bits.slice(0);
	return result;
}

BinaryBlob.prototype.getBit = function(bit) {
	return this.bits[bit];
}

BinaryBlob.prototype.setBit = function(bit, value) {
	this.bits[bit] = value?1:0;
}

BinaryBlob.prototype.getRange = function(lsb, msb) {
	var result = new BinaryBlob((msb-lsb)+1);
	for (var i=lsb, j=0; i<= msb; i++,j++) {
		result.bits[j] = this.bits[i] ? 1:0;
	}
	return result;
}

BinaryBlob.prototype.setRange = function(start, blob) {
	console.assert(start+blob.size <= this.size);
	console.assert(start >= 0);
	for (var i=start, j=0; j < blob.size; i++,j++) {
		this.bits[i] = blob.bits[j] ? 1:0;
	}
}

BinaryBlob.prototype.toInt = function() {
	var result = 0;
	for (var i=0; i<this.size; ++i) {
		if (this.bits[i]) {
			result |= (1<<i);
		}
	}
	return result;
}

BinaryBlob.prototype.toBinaryString = function() {
	var result = '';
	for (var i=this.size-1; i>=0; --i) {
		result += (this.bits[i])?'1':'0';
	}
	return result;
}

BinaryBlob.prototype.toHexString = function() {
	var result = '';
	var digits = '0123456789ABCDEF';
	var cur = 0;
	while (cur < this.size)
	{
		var b0 = this.bits[cur];
		var b1 = 0, b2 = 0, b3 = 0;
		if (cur+1 < this.size) {
			b1 = this.bits[cur+1];
		}
		if (cur+2 < this.size) {
			b2 = this.bits[cur+2];
		}
		if (cur+3 < this.size) {
			b3 = this.bits[cur+3];
		}
		var nibble = b0 | (b1<<1) | (b2<<2) | (b3<<3);
		result = digits.charAt(nibble) + result;
		cur +=4;
	}
	return result;
}
