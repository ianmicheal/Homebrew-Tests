#include "audio_assist.h"

ALboolean al_init(){
	ALboolean enumeration;
	const ALCchar *defaultDeviceName;
	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	ALCenum error = AL_FALSE;

	enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if(enumeration == AL_FALSE){
		fprintf(stderr, "enumeration extension not available\n");
	}

	list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

	// if(!defaultDeviceName){
		defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	// }

	al_device = alcOpenDevice(defaultDeviceName);
	if(!al_device){
		fprintf(stderr, "unable to open default device\n");
		return AL_FALSE;
	}

    // fprintf(stdout, "Device: %s\n", alcGetString(device, ALC_DEVICE_SPECIFIER));

	alGetError();

	al_context = alcCreateContext(al_device, NULL);
	if(!alcMakeContextCurrent(al_context)){
		fprintf(stderr, "failed to make default context\n");
		return AL_FALSE;
	}
	al_test_error(&error, "make default context");
	if(error == AL_TRUE){return AL_FALSE;}

	// set orientation
	alListener3f(AL_POSITION, 0, 0, 1.0f);
	al_test_error(&error, "listener position");
	if(error == AL_TRUE){return AL_FALSE;}

    alListener3f(AL_VELOCITY, 0, 0, 0);
	al_test_error(&error, "listener velocity");
	if(error == AL_TRUE){return AL_FALSE;}

	alListenerfv(AL_ORIENTATION, listenerOri);
	al_test_error(&error, "listener orientation");
	if(error == AL_TRUE){return AL_FALSE;}

	return AL_TRUE;
}

void al_shutdown(){
	// alDeleteSources(1, &source);
	// alDeleteBuffers(1, &buffer);
	al_device = alcGetContextsDevice(al_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(al_context);
	alcCloseDevice(al_device);
	return;
}


ALboolean al_test_error(ALCenum * error, char * msg){
	*error = alGetError();
	if(*error != AL_NO_ERROR){
        fprintf(stderr, "ERROR: ");
		fprintf(stderr, msg);
		fprintf(stderr, "\n");
		return AL_TRUE;
	}
	return AL_FALSE;
}

void list_audio_devices(const ALCchar *devices){
	const ALCchar *device = devices, *next = devices + 1;
	size_t len = 0;

	fprintf(stdout, "Devices list:\n");
	fprintf(stdout, "----------\n");
	while(device && *device != '\0' && next && *next != '\0'){
		fprintf(stdout, "%s\n", device);
		len = strlen(device);
		device += (len + 1);
		next += (len + 2);
	}
	fprintf(stdout, "----------\n");
}

inline ALenum to_al_format(short channels, short samples){
	bool stereo = (channels > 1);

	switch(samples){
	case 16:
		if(stereo){
			return AL_FORMAT_STEREO16;
		}
		else{
			return AL_FORMAT_MONO16;
		}
	case 8:
		if(stereo){
			return AL_FORMAT_STEREO8;
		}
		else{
			return AL_FORMAT_MONO8;
		}
	default:
		return -1;
	}
}

bool is_big_endian(){
	int a = 1;
	return !((char*)&a)[0];
}

int convert_to_int(char * buffer, int len){
	int i = 0;
	int a = 0;
	if(!is_big_endian()){
		for(; i<len; i++){
			((char*)&a)[i] = buffer[i];
		}
	}
	else{
		for(; i<len; i++){
			((char*)&a)[3 - i] = buffer[i];
		}
	}
	return a;
}

// void WAVE_buffer(void *data){
	// int spare = WAV_size - ftell(in);
	// int read = fread(data, 1, MIN(DATA_CHUNK_SIZE, spare), in);
	// printf("Reading WAV: read: %d spare:%d\n", read, spare);
	// if(read < DATA_CHUNK_SIZE){
	// 	printf("Writing silence\n");
	// 	fseek(in, WAV_data_start, SEEK_SET);
	// 	memset(&((char *)data)[read], 0, DATA_CHUNK_SIZE - read);
	// 	//fread(&((char*)data)[read], DATA_CHUNK_SIZE-read, 1, in);
	// }
// }

ALboolean LoadWAVFile(const char * filename, al_audio_data_t * audio_data, uint8_t mode){
	audio_data->play_type = mode;
	uint16_t length = strlen(filename);
	switch(mode){
		case AL_AS_TYPE_NON_STREAM:
			audio_data->is_cdda = 0;
			// audio_data->fp = NULL;
			audio_data->path = malloc(sizeof(char) * length);
			strcpy(audio_data->path, filename);
		break;
		case AL_AS_TYPE_STREAM:
			audio_data->is_cdda = 0;
			// audio_data->fp = NULL;
			audio_data->path = malloc(sizeof(char) * length);
			strcpy(audio_data->path, filename);
		break;
		case AL_AS_TYPE_CDDA:
			audio_data->is_cdda = 1;
			// audio_data->fp = NULL;
			audio_data->path = NULL;
		break;
		default:
			fprintf(stderr, "Invalid wav load mode given\n");
			return AL_FALSE;
		break;
	}

	char buffer[4];

	FILE* in = fopen(filename, "rb");
	if(!in){
		fprintf(stderr, "Couldn't open file\n");
		return AL_FALSE;
	}

	fread(buffer, 4, sizeof(char), in);

	if(strncmp(buffer, "RIFF", 4) != 0){
		fprintf(stderr, "Not a valid wave file\n");
		return AL_FALSE;
	}

	fread(buffer, 4, sizeof(char), in);
	fread(buffer, 4, sizeof(char), in);		//WAVE
	fread(buffer, 4, sizeof(char), in);		//fmt
	fread(buffer, 4, sizeof(char), in);		//16
	fread(buffer, 2, sizeof(char), in);		//1
	fread(buffer, 2, sizeof(char), in);

	int chan = convert_to_int(buffer, 2);
	fread(buffer, 4, sizeof(char), in);
	audio_data->freq = convert_to_int(buffer, 4);
	fread(buffer, 4, sizeof(char), in);
	fread(buffer, 2, sizeof(char), in);
	fread(buffer, 2, sizeof(char), in);
	int bps = convert_to_int(buffer, 2);
	fread(buffer, 4, sizeof(char), in);		//data
	fread(buffer, 4, sizeof(char), in);
	audio_data->size = (ALsizei) convert_to_int(buffer, 4);
	audio_data->data = (ALvoid*) malloc(audio_data->size * sizeof(char));
	fread(audio_data->data, audio_data->size, sizeof(char), in);

	if(chan == 1){
		audio_data->format = (bps == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
	}
	else{
		audio_data->format = (bps == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
	}

	fclose(in);

	printf("Loaded WAV file!\n");

	return AL_TRUE;
}