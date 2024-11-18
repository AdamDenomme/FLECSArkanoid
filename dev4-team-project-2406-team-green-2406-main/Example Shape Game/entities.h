// entities.h creates the data used by the game

// The game is stored in here (very efficiently)
entt::registry registry; // globals are not ideal but this is the only one

// Functions to Create Entities:

bool CreateCursor(GameLoop loop) {
	// create a small orange circle to be locked to our mouse
	auto entity = registry.create();
	registry.emplace<COLOR>(entity, GW::MATH::GVECTORF{ 0.990f, 0.578f, 0.0396f, 1 });
	registry.emplace<CIRCLE>(entity, GW::MATH2D::GCIRCLE2F{ 0, 0, 0.05f });
	registry.emplace<GW::MATH2D::GVECTOR2F>(entity); // previous X,Y location of the 3D cursor
	// add a component with input system and event cache to control the cursor
	GW::INPUT::GBufferedInput mouse;
	if (-mouse.Create(loop.window))
		return false;
	GW::CORE::GEventCache events;
	if (-events.Create(256) || -mouse.Register(events))
		return false;
	// label this as a cursor
	registry.emplace<CURSOR>(entity, 
		CURSOR{ CURSOR::MODE::HIGHLIGHT, mouse.Relinquish(), events.Relinquish() });

	return true;
}

// could not #include <random> for some reason so using a more basic approach
#define RAND_FLOAT(min, max) (((max)-(min))*(rand())/float(RAND_MAX)+(min))
// Spawn a series of random shapes in a specified range
bool CreateRandomShapes(unsigned int count, float x_range, float y_range, 
						float min_scale, float max_scale) {
	srand(time(NULL));
	// loop for count, spawning random shapes
	while (count--) {
		// construct a new entity
		auto entity = registry.create();
		registry.emplace<COLOR>(entity);
		registry.emplace<GAME_OBJECT>(entity);
		// select a random staring position and size
		float x_loc = RAND_FLOAT(-x_range, x_range);
		float y_loc = RAND_FLOAT(-y_range, y_range);
		float scale = RAND_FLOAT(min_scale, max_scale);
		float angle = RAND_FLOAT(0, G_PI_F);
		int type = rand() % 3; // line, circle, rectangle
		LINE l; CIRCLE c; RECTANGLE r;
		// pick a random shape type
		switch (type) {
		case 0: // LINE
			l.start.x = x_loc - cosf(angle) * scale;
			l.start.y = y_loc - sinf(angle) * scale;
			l.end.x = x_loc + cosf(angle) * scale;
			l.end.y = y_loc + sinf(angle) * scale;
			registry.emplace<LINE>(entity, l);
			break;
		case 1: // CIRCLE
			c.pos = { x_loc, y_loc };
			c.radius = scale;
			registry.emplace<CIRCLE>(entity, c);
			break;
		case 2: // RECTANGLE
			r.min = { x_loc - scale, y_loc - scale };
			r.max = { x_loc + scale, y_loc + scale };
			registry.emplace<RECTANGLE>(entity, r);
			break;
		default:
			return false;
		};
	}
	return true;
}

// Create the debug renderer for our collidable shapes
bool CreateDebugRenderer(GraphicsEngine engine) {
	// initialize everything the debug renderer needs
	DebugRenderer debugger;
	// Set initial camera and projection
	float ar = 1;
	engine.surface.GetAspectRatio(ar);
	GW::MATH::GMatrix::LookAtLHF( GW::MATH::GVECTORF{ 0,0,-10,0 }, // results in view
		GW::MATH::GVECTORF{ 0,0,0,0 }, GW::MATH::GVECTORF{ 0,1,0,0 }, debugger.camera);
	GW::MATH::GMatrix::ProjectionDirectXLHF(G_DEGREE_TO_RADIAN_F(75),
		ar, 0.01, 100, debugger.projection);
	//combine matricies
	GW::MATH::GMatrix::MultiplyMatrixF(debugger.camera, 
		debugger.projection, debugger.viewProjection);
	// switch view to camera
	GW::MATH::GMatrix::InverseF(debugger.camera, debugger.camera);
	// load compiled shaders into buffers
	const char* shaders[] = {
		"../DebugVS.cso", // ideally pulled from a .ini file
		"../DebugPS.cso" // ideally pulled from a .ini file
	};
	std::vector<char> buffers[ARRAYSIZE(shaders)];
	for (int i = 0; i < ARRAYSIZE(shaders); ++i) {
		// load and copy file data to buffer
		GW::SYSTEM::GFile file;
		if (-file.Create() || -file.OpenBinaryRead(shaders[i]))
			return false;
		unsigned int len = 0;
		if (-file.GetFileSize(shaders[i], len))
			return false;
		buffers[i].resize(len);
		if (-file.Read(buffers[i].data(), len))
			return false;
	}
	// all shaders should be loaded from disk lets create them
	ID3D11Device* creator;
	if (-engine.surface.GetDevice((void**)&creator))
		return false;
	// VS shader
	creator->CreateVertexShader(buffers[0].data(),
		buffers[0].size(), nullptr, debugger.vertexShader.GetAddressOf());
	// PS shader
	creator->CreatePixelShader(buffers[1].data(),
		buffers[1].size(), nullptr, debugger.pixelShader.GetAddressOf());
	// now we can create the specific input layout used by this renderer
	D3D11_INPUT_ELEMENT_DESC format[] =
	{
		{ "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "CLR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	creator->CreateInputLayout(format, ARRAYSIZE(format),
		buffers[0].data(), buffers[0].size(), debugger.vertexFormat.GetAddressOf());
	// allocate constant buffer
	D3D11_SUBRESOURCE_DATA cbData = { &debugger.viewProjection, 0, 0 };
	CD3D11_BUFFER_DESC cbDesc(sizeof(GW::MATH::GMATRIXF), D3D11_BIND_CONSTANT_BUFFER);
	creator->CreateBuffer(&cbDesc, nullptr, debugger.constantBuffer.GetAddressOf());

	// the vertex buffer will be allocated on demand later so we ignore it for now
	// transfer the debug renderer into the EnTT registry
	auto entity = registry.create();
	registry.emplace<DebugRenderer>(entity, std::move(debugger));

	creator->Release(); // free device handle
	return true;
}

// make the main d3d11 graphics engine used by the renderers
bool CreateGraphicsEngine(GameLoop loop) {
	GraphicsEngine engine;
	if (-engine.surface.Create(loop.window, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		return false;
	// create any renderers used by the game
	if (CreateDebugRenderer(engine) == false)
		return false;
	// transfer ownership to EnTT
	auto entity = registry.create();
	registry.emplace<GraphicsEngine>(entity, std::move(engine));

	return true;
}

// makes a GameLoop entity
bool CreateGameLoop() {
	// create main GameLoop
	GameLoop loop;
	if (-loop.window.Create(0, 0, 800, 600, GW::SYSTEM::GWindowStyle::FULLSCREENBORDERLESS))
		return false;
	auto entity = registry.create();
	// create graphics engine
	if (CreateGraphicsEngine(loop) == false)
		return false;
	// Create random game entities (ideally would read from .ini file)
	if (CreateRandomShapes(100, 12, 5, 0.25, 1.5) == false)
		return false;
	// Create our Cursor
	if (CreateCursor(loop) == false)
		return false;

	// transfer ownership to EnTT
	registry.emplace<GameLoop>(entity, std::move(loop));

	return true;
}


