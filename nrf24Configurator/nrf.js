
//------------------------- Register ------------------------------------------

function Register(addr, name, size, resetValue) {
	this.addr = addr;
	this.name = name;
	this.size = size;
	console.assert(size == resetValue.size);
	this.resetValue = resetValue;
	this.value = this.resetValue.copy();
	this.fieldList = [];
	this.fieldMap = {};
}

Register.prototype.addField = function(field) {
	this.fieldList.push(field);
	this.fieldMap[field.name] = field;
	field.register = this;
}

//--------------------------- Field ------------------------------------------

function Field(name, lsb, msb, options) {
	if (msb === undefined) {
		msb = lsb;
	}
	console.assert(msb >= lsb);
	console.assert(lsb >= 0);
	options = options || {};
	this.register = null;
	this.name = name;
	this.lsb = lsb;
	this.msb = msb;
	this.size = (this.msb - this.lsb)+1;
	this.hex = !!options.hex;
}

Field.prototype.getBits = function() {
	return this.register.value.getRange(this.lsb, this.msb);
}

Field.prototype.setBits = function(value) {
	this.register.value.setRange(this.lsb, value);
}

var registerList = [];
var fieldMap = {};
var fieldList = [];

function addRegister(reg) {
	registerList.push(reg);
}

function addField(field) {
	registerList[registerList.length-1].addField(field);
	fieldMap[field.name] = field;
	fieldList.push(field);
}

fromBinary = BinaryBlob.fromBinaryString;
fromHex = BinaryBlob.fromHexString;

addRegister(new Register(0x00, 'CONFIG', 8, fromBinary('0000_1000')));
addField(new Field('Reserved', 7));
addField(new Field('MASK_RX_DR', 6));
addField(new Field('MASK_TX_DS', 5));
addField(new Field('MASK_MAX_RT', 4));
addField(new Field('EN_CRC', 3));
addField(new Field('CRCO', 2));
addField(new Field('PWR_UP', 1));
addField(new Field('PRIM_RX', 0));

addRegister(new Register(0x01, 'EN_AA', 8, fromBinary('0011_1111')));
addField(new Field('Reserved', 6,7));
addField(new Field('ENAA_P5', 5));
addField(new Field('ENAA_P4', 4));
addField(new Field('ENAA_P3', 3));
addField(new Field('ENAA_P2', 2));
addField(new Field('ENAA_P1', 1));
addField(new Field('ENAA_P0', 0));

addRegister(new Register(0x02, 'EN_RXADDR', 8, fromBinary('0000_0011')));
addField(new Field('Reserved', 6,7));
addField(new Field('ERX_P5', 5));
addField(new Field('ERX_P4', 4));
addField(new Field('ERX_P3', 3));
addField(new Field('ERX_P2', 2));
addField(new Field('ERX_P1', 1));
addField(new Field('ERX_P0', 0));

addRegister(new Register(0x03, 'SETUP_AW', 8, fromBinary('0000_0011')));
addField(new Field('Reserved', 2,7));
addField(new Field('AW', 0,1));

addRegister(new Register(0x04, 'SETUP_RETR', 8, fromBinary('0000_0011')));
addField(new Field('ARD', 4,7));
addField(new Field('ARC', 0,3));

addRegister(new Register(0x05, 'RF_CH', 8, fromBinary('0000_0010')));
addField(new Field('Reserved', 7));
addField(new Field('RF_CH', 0,6));

addRegister(new Register(0x06, 'RF_SETUP', 8, fromBinary('0000_1110')));
addField(new Field('CONT_WAVE', 7));
addField(new Field('Reserved', 6));
addField(new Field('RF_DR_LOW', 5));
addField(new Field('PLL_LOCK', 4));
addField(new Field('RF_DR_HIGH', 3));
addField(new Field('RF_PWR', 1,2));
addField(new Field('Obsolete', 0));

addRegister(new Register(0x0A, 'RX_ADDR_P0', 40, fromHex('E7E7E7E7E7')));
addField(new Field('RX_ADDR_P0', 0,39, {hex:true}));

addRegister(new Register(0x0B, 'RX_ADDR_P1', 40, fromHex('C2C2C2C2C2')));
addField(new Field('RX_ADDR_P1', 0,39, {hex:true}));

addRegister(new Register(0x0C, 'RX_ADDR_P2', 8, fromHex('C3')));
addField(new Field('RX_ADDR_P2', 0,7, {hex:true}));

addRegister(new Register(0x0D, 'RX_ADDR_P3', 8, fromHex('C4')));
addField(new Field('RX_ADDR_P3', 0,7, {hex:true}));

addRegister(new Register(0x0E, 'RX_ADDR_P4', 8, fromHex('C5')));
addField(new Field('RX_ADDR_P4', 0,7, {hex:true}));

addRegister(new Register(0x0F, 'RX_ADDR_P5', 8, fromHex('C6')));
addField(new Field('RX_ADDR_P5', 0,7, {hex:true}));

addRegister(new Register(0x10, 'TX_ADDR', 40, fromHex('E7E7E7E7E7')));
addField(new Field('TX_ADDR', 0,39, {hex:true}));

addRegister(new Register(0x11, 'RX_PW_P0', 8, fromHex('00')));
addField(new Field('Reserved', 6,7));
addField(new Field('RX_PW_P0', 0,5));

addRegister(new Register(0x12, 'RX_PW_P1', 8, fromHex('00')));
addField(new Field('Reserved', 6,7));
addField(new Field('RX_PW_P1', 0,5));

addRegister(new Register(0x13, 'RX_PW_P2', 8, fromHex('00')));
addField(new Field('Reserved', 6,7));
addField(new Field('RX_PW_P2', 0,5));

addRegister(new Register(0x14, 'RX_PW_P3', 8, fromHex('00')));
addField(new Field('Reserved', 6,7));
addField(new Field('RX_PW_P3', 0,5));

addRegister(new Register(0x15, 'RX_PW_P4', 8, fromHex('00')));
addField(new Field('Reserved', 6,7));
addField(new Field('RX_PW_P4', 0,5));

addRegister(new Register(0x16, 'RX_PW_P5', 8, fromHex('00')));
addField(new Field('Reserved', 6,7));
addField(new Field('RX_PW_P5', 0,5));

addRegister(new Register(0x1C, 'DYNPD', 8, fromHex('00')));
addField(new Field('Reserved', 6,7));
addField(new Field('DPL_P5', 5));
addField(new Field('DPL_P4', 4));
addField(new Field('DPL_P3', 3));
addField(new Field('DPL_P2', 2));
addField(new Field('DPL_P1', 1));
addField(new Field('DPL_P0', 0));

addRegister(new Register(0x1D, 'FEATURE', 8, fromHex('00')));
addField(new Field('Reserved', 3,7));
addField(new Field('EN_DPL', 2));
addField(new Field('EN_ACK_PAY', 1));
addField(new Field('EN_DYN_ACK', 0));

var updateFunctions = [];

function addCheckbox(text, fieldName, inverted) {
	var checkbox = $('<input type="checkbox">');
	$("#f1").append(checkbox);
	checkbox.after(' '+text + ' ['+fieldName+'='+(inverted?0:1)+']<br>');

	var field = fieldMap[fieldName];
	var suppress = [0];

	checkbox.change(function() {
		var isChecked = checkbox.prop('checked');
		if (inverted) {
			isChecked = !isChecked;
		}
		field.setBits(BinaryBlob.fromInt(isChecked?1:0, 1));
		suppress[0]=1;
		updateAll();
		suppress[0]=0;
	});

	updateFunctions.push(function() {
		if(!suppress[0]) {
			var value = field.getBits().getBit(0);
			checkbox.prop('checked', inverted ? ! value : value);
		}
	});
}

function addDropdown(fieldName, options) {
	var sel = $('<select>');
	$('#f1').append(sel).append('<br>');

	var field = fieldMap[fieldName];
	for (var i=0; i<options.length; ++i) {
		sel.append('<option value="'+i+'">'+options[i]+' ['+fieldName+'='+
			BinaryBlob.fromInt(i, field.size).toBinaryString()+
			']</option>');
	}

	var suppress = [0];

	sel.change(function() {
		field.setBits(BinaryBlob.fromInt(sel.val(), field.size));
		suppress[0]=1;
		updateAll();
		suppress[0]=0;
	});

	updateFunctions.push(function() {
		if(!suppress[0]) {
			sel.val(field.getBits().toInt());
		}
	});
}

function addHexEntry(desc, fieldName, options) {
	var options = options || {};

	var textEntry = $('<input type="text"></input>');
	$('#f1').append(desc+': ');

	if (options.addrLSB) {
		var addrPrefixSpan = $('<span></span>');
		$('#f1').append(addrPrefixSpan);
	}

	$('#f1').append(textEntry).append('<br>');


	var field = fieldMap[fieldName];
	textEntry.attr('size', Math.ceil(field.size/4));
	textEntry.attr('maxlength', Math.ceil(field.size/4));

	var suppress = [0];

	textEntry.change(function() {
		var parseOK = true;
		try 
		{
			var parsedBits = BinaryBlob.fromHexString(textEntry.val(), field.size);
		} catch(e) {
			parseOK = false;
		}

		if (parseOK) {
			textEntry.removeClass('invalid');
			field.setBits(parsedBits);
			textEntry.val(field.getBits().toHexString());
			suppress[0]=1;
			updateAll();
			suppress[0]=0;
		} else {
			textEntry.addClass('invalid');
		}
	});

	updateFunctions.push(function() {
		if(!suppress[0]) {
			textEntry.removeClass('invalid');
			textEntry.val(field.getBits().toHexString());
		}
		if (options.addrLSB) {
			$(addrPrefixSpan).text(fieldMap['RX_ADDR_P1'].getBits().toHexString().slice(0,4*2));
		}
	});
}

function addLogHeader(text) {
	$('#f1').append('<h2>'+text+'</h2>');
}

addLogHeader('General');
addCheckbox('Power up module', 'PWR_UP');
addDropdown('PRIM_RX',
			['Primary transmitter',
			 'Primary receiver']);

addLogHeader('Features');
addCheckbox('Enable CRC', 'EN_CRC');
addDropdown('CRCO',
			['1 byte CRC',
			 '2 byte CRC']);
addDropdown('AW',
			['(illegal)',
			 '3 byte addresses',
			 '4 byte addresses',
			 '5 byte addresses'
			 ]);
addDropdown('ARC',
			function() {
				var result = [];
				result.push('Re-transmit disabled');
				for (var i=1; i<=0xF; ++i) {
					var plural = (i==1)?"":"s";
					result.push('Up to '+i+' re-transmit'+plural+' upon auto-ack failure');
				}
				return result;
			}());

addDropdown('ARD',
			function() {
				var result = [];
				for (var i=0; i<=0xF; ++i) {
					var usecs = (i+1)*250;
					result.push(usecs+' &micro;s auto re-transmit delay');
				}
				return result;
			}());

addCheckbox('Enable dynamic payload length', 'EN_DPL');
addCheckbox('Enable payload in ACK packet', 'EN_ACK_PAY');
addCheckbox('Enable the W_TX_PAYLOAD_NOACK command', 'EN_DYN_ACK');

addLogHeader('RF');
addDropdown('RF_CH',
			function() {
				var result = [];
				for (var i=0; i<=0x7F; ++i) {
					var mhz = 2400 + i;
					result.push('Channel: '+mhz+' MHz');
				}
				return result;
			}());
// TODO: RF_DR (data rate)
addDropdown('RF_PWR',
			[
				'TX output power: -18 dBm',
				'TX output power: -12 dBm',
				'TX output power: -6 dBm',
				'TX output power: 0 dBm',
			]);
addCheckbox('Enable continuous carrier transmit (for testing)', 'CONT_WAVE');
addCheckbox('Force PLL lock signal (for testing)', 'PLL_LOCK');

addLogHeader('Interrupts');
addCheckbox('Reflect RX_DR status bit on active low IRQ line', 'MASK_RX_DR',true);
addCheckbox('Reflect TX_DS status bit on active low IRQ line', 'MASK_TX_DS',true);
addCheckbox('Reflect MAX_RT status bit on active low IRQ line', 'MASK_MAX_RT',true);

addLogHeader('TX');
addHexEntry('TX address', 'TX_ADDR');

for (var i=0; i<6; ++i) {
	addLogHeader('RX Pipe '+i);
	addCheckbox('Enable', 'ERX_P'+i);
	addHexEntry('RX address', 'RX_ADDR_P'+i, {addrLSB: i > 1});
	addCheckbox('Auto-ack', 'ENAA_P'+i);
	addCheckbox('Dynamic payload length', 'DPL_P'+i);
}

function addRegisterSummary(regTable, reg) {
	var row = $('<tr><td>'+
				BinaryBlob.fromInt(reg.addr,8).toHexString()+
				'</td><td>'+reg.name+'</td><td></td><td></td></tr>');
	var valueTD = row.find('td')[2];
	var valueBitsTD = row.find('td')[3];
	regTable.append(row);

	var innerTable = $('<table></table>');

	var topRow='',midRow='',bottomRow='';

	var bitClass = "bit";
	if (reg.size > 8) {
		bitClass = "smallbit";
	}
	
	var bitno = reg.size-1;
	for (var i=0; i< reg.fieldList.length; ++i) {
		var field = reg.fieldList[i];
		var altClass = (reg.fieldList.length-i-1)%2 ? ' alt':'';

		midRow += '<td class="fieldname'+altClass+'" colspan="'+field.size+'">'+field.name+'</td>'

		for (var j=0; j<field.size; ++j) {
			topRow += "<td class=\""+bitClass+altClass+"\">"+bitno+"</td>"
			bottomRow += '<td class="'+altClass+'"><input type="checkbox"></td>';

			--bitno;
		}
	}

	innerTable.append($("<tr>"+topRow+"</tr>"));
	innerTable.append($("<tr>"+midRow+"</tr>"));
	innerTable.append($("<tr>"+bottomRow+"</tr>"));

	$(valueBitsTD).append(innerTable);

	updateFunctions.push(function() {
		$(valueTD).text(reg.value.toHexString());

		for (var i=0; i<reg.size; ++i) {
			var bitNo = reg.size-i-1;
			var bitVal = reg.value.getBit(bitNo);
			var checkbox = innerTable.find('input')[i];
			$(checkbox).prop('checked', !!bitVal);
		}
	});
}

$("#regs").append('<h2>Summary</h2>');
var regTable = $("<table></table>");
regTable.append('<tr><th>Addr (hex)</th><th>Register</th><th>Value (hex)</th><th>Value (bits)</th></tr>');
for (var i=0; i<registerList.length; ++i) {
	var reg = registerList[i];
	addRegisterSummary(regTable, reg);	
}
$("#regs").append(regTable);

// function addFieldDetail(table, field) {
// 	var bitRange = ""+field.msb;
// 	if (field.msb != field.lsb) {
// 		bitRange = field.msb+":"+field.lsb;
// 	}
// 	var row = $('<tr><td>'+field.name+'</td><td>'+bitRange+'</td><td></td></tr>');
// 	var valueTD = row.find('td')[2];	table.append(row);

// 	updateFunctions.push(function() {
// 		if (field.hex) {
// 			$(valueTD).text(field.getBits().toHexString());
// 		} else {
// 			$(valueTD).text(field.getBits().toBinaryString());
// 		}
// 	})
// }

// for (var i=0; i<registerList.length; ++i) {
// 	var reg = registerList[i];
// 	$("#regs").append('<h2>0x' + BinaryBlob.fromInt(reg.addr,8).toHexString() +' '+reg.name+'</h2>');
// 	var table = $("<table></table>");
// 	table.append('<tr><th>Field</th><th>Bit</th><th>Value</th></tr>');
// 	for (var j=0; j<reg.fieldList.length; ++j) {
// 		var field = reg.fieldList[j];
// 		addFieldDetail(table, field);
// 	}
// 	$("#regs").append(table);
// }

function updateAll() {
	for (var i=0; i<updateFunctions.length; ++i) {
		updateFunctions[i]();
	}
}

updateAll();
