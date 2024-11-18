// systems.h processes the data used in the game

// compute the hit count of each object and its neighbors
bool ProcessGameObjects() {
	// clear all hit components
	registry.clear<HIT_COUNT>();
	// grab views to all game objects
	auto lineView = registry.view<LINE, GAME_OBJECT>();
	auto circleView = registry.view<CIRCLE, GAME_OBJECT>();
	auto rectangleView = registry.view<RECTANGLE, GAME_OBJECT>();

	// *NOTE* This approach is a brute force. [Exponential time: O(2^n)]
	// It simply tests all shapes against all other shapes.
	// This might be ok for a few shapes, but it scales up horribly!
	// Ex: 100 objects = 10,000 tests per frame. 1000 = 1,000,000 tests per frame!
	// If you are curious how games/engines handle these problems efficiently:
	// https://gameprogrammingpatterns.com/spatial-partition.html 

	// collide objects against themselves
	for (auto i = lineView.begin(); i != lineView.end(); i++) {
		auto one = registry.get<LINE>(*i);
		auto j = i; // these iterators have no pre-increment
		for (j++; j != lineView.end(); j++) {
			auto two = registry.get<LINE>(*j);
			GW::MATH2D::GCollision2D::GCollisionCheck2D result;
			GW::MATH2D::GCollision2D::TestLineToLine2F(one, two, result);
			if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION) {
				++registry.get_or_emplace<HIT_COUNT>(*i).hits;
				++registry.get_or_emplace<HIT_COUNT>(*j).hits;
			}
		}
	}
	for (auto i = circleView.begin(); i != circleView.end(); i++) {
		auto one = registry.get<CIRCLE>(*i);
		auto j = i; // these iterators have no pre-increment
		for (j++; j != lineView.end(); j++) {
			auto two = registry.get<CIRCLE>(*j);
			GW::MATH2D::GCollision2D::GCollisionCheck2D result;
			GW::MATH2D::GCollision2D::TestCircleToCircle2F(one, two, result);
			if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION) {
				++registry.get_or_emplace<HIT_COUNT>(*i).hits;
				++registry.get_or_emplace<HIT_COUNT>(*j).hits;
			}
		}
	}
	for (auto i = rectangleView.begin(); i != rectangleView.end(); i++) {
		auto one = registry.get<RECTANGLE>(*i);
		auto j = i; // these iterators have no pre-increment
		for (j++; j != rectangleView.end(); j++) {
			auto two = registry.get<RECTANGLE>(*j);
			GW::MATH2D::GCollision2D::GCollisionCheck2D result;
			GW::MATH2D::GCollision2D::TestRectangleToRectangle2F(one, two, result);
			if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION) {
				++registry.get_or_emplace<HIT_COUNT>(*i).hits;
				++registry.get_or_emplace<HIT_COUNT>(*j).hits;
			}
		}
	}
	// collide lines against rectangles
	for (auto i = lineView.begin(); i != lineView.end(); i++) {
		auto one = registry.get<LINE>(*i);
		for (auto j = rectangleView.begin(); j != rectangleView.end(); j++) {
			auto two = registry.get<RECTANGLE>(*j);
			GW::MATH2D::GCollision2D::GCollisionCheck2D result;
			// hmm... this seems a bit iffy, need to file a bug report
			GW::MATH2D::GCollision2D::TestLineToRectangle2F(one, two, result);
			if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION) {
				++registry.get_or_emplace<HIT_COUNT>(*i).hits;
				++registry.get_or_emplace<HIT_COUNT>(*j).hits;
			}
		}
	}
	// collide lines against circles
	for (auto i = lineView.begin(); i != lineView.end(); i++) {
		auto one = registry.get<LINE>(*i);
		for (auto j = circleView.begin(); j != circleView.end(); j++) {
			auto two = registry.get<CIRCLE>(*j);
			GW::MATH2D::GCollision2D::GCollisionCheck2D result;
			GW::MATH2D::GCollision2D::TestLineToCircle2F(one, two, result);
			if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION) {
				++registry.get_or_emplace<HIT_COUNT>(*i).hits;
				++registry.get_or_emplace<HIT_COUNT>(*j).hits;
			}
		}
	}
	// collide rectangles against circles
	for (auto i = rectangleView.begin(); i != rectangleView.end(); i++) {
		auto one = registry.get<RECTANGLE>(*i);
		GW::MATH2D::GLINE2F top = { one.max, {one.min.x, one.max.y} };
		GW::MATH2D::GLINE2F bottom = { one.min, {one.max.x, one.min.y} };
		GW::MATH2D::GLINE2F left = { one.min, {one.min.x, one.max.y} };
		GW::MATH2D::GLINE2F right = { one.max, {one.max.x, one.min.y} };
		for (auto j = circleView.begin(); j != circleView.end(); j++) {
			auto two = registry.get<CIRCLE>(*j);
			GW::MATH2D::GCollision2D::GCollisionCheck2D results[4];
			// No built in Circle to Rectangle is an oversight, feature request it.
			GW::MATH2D::GCollision2D::TestLineToCircle2F(top, two, results[0]);
			GW::MATH2D::GCollision2D::TestLineToCircle2F(bottom, two, results[1]);
			GW::MATH2D::GCollision2D::TestLineToCircle2F(left, two, results[2]);
			GW::MATH2D::GCollision2D::TestLineToCircle2F(right, two, results[3]);
			if (results[0] == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION ||
				results[1] == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION ||
				results[2] == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION ||
				results[3] == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION) {
				++registry.get_or_emplace<HIT_COUNT>(*i).hits;
				++registry.get_or_emplace<HIT_COUNT>(*j).hits;
			}
		}
	}
	return true;
}

bool ProcessColors() {
	// reset/clear all existing game object colors to white
	auto wipeView = registry.view<COLOR, GAME_OBJECT>();
	for (auto [ent, clr] : wipeView.each()) {
		clr = GW::MATH::GVECTORF{ 1,1,1,1 };
	}
	// based on the count of collisions adjust colors 
	const GW::MATH::GVECTORF colors[] = {
		// cyan, green, blue, red, brown, purple 
		{0,1,1,1}, {0,1,0,1}, {0,0,1,1}, {1,0,0,1},
		{0.670f, 0.357f, 0.00f, 1}, {0.584f, 0.00f, 0.760f, 1}
	};
	// traverse all objects with hit count and apply color
	auto hitView = registry.view<COLOR, HIT_COUNT>();
	for (auto [ent, clr, hit] : hitView.each()) {
		clr = colors[G_CLAMP(hit.hits-1, 0, ARRAYSIZE(colors)-1)];
	}
	// after highlighting or selecting all objects change the colors
	// Set all highlighted objects to yellow
	auto highlightView = registry.view<COLOR, HIGHLIGHTED>();
	for (auto [ent, clr] : highlightView.each()) {
		clr = GW::MATH::GVECTORF{ 1,1,0,1 };
	}
	// Set all selected objects to orange
	auto selectedView = registry.view<COLOR, SELECTED>();
	for (auto [ent, clr] : selectedView.each()) {
		clr = GW::MATH::GVECTORF{ 0.990f, 0.578f, 0.0396f, 1 };
	}
	return true;
}

// Process any cursors used to highlight/move objects
bool ProcessCursors(GameLoop& loop) {
	// grab window dimensions
	unsigned width = 0, height = 0;
	if (-loop.window.GetClientWidth(width) || 
		-loop.window.GetClientHeight(height))
		return false;
	// Grab the first DebugRenderer we find to get the viewProjection inverse
	auto &renderer = registry.get<DebugRenderer>(
		registry.view<DebugRenderer>().front());
	GW::MATH::GMATRIXF invViewProj = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMatrix::InverseF(renderer.viewProjection, invViewProj);
	// wipe any existing highlighted items (they will be re-determined)
	registry.clear<HIGHLIGHTED>();
	// process any and all cursors
	auto view = registry.view<CIRCLE, COLOR, CURSOR, GW::MATH2D::GVECTOR2F>();
	for (auto [ent, cir, clr, cur, prev] : view.each()) {
		// variables used later
		bool mousePressed = false;
		bool mouseReleased = false;
		// scan the event cache for relevant input events and update cursor
		GW::GEvent event;
		while (+cur.events.Pop(event)) {
			GW::INPUT::GBufferedInput::Events e;
			GW::INPUT::GBufferedInput::EVENT_DATA d;
			if (+event.Read(e, d)) {
				switch (e) {
				// When clicking upgrade any highlighted to selected
				case GW::INPUT::GBufferedInput::Events::BUTTONPRESSED:
					mousePressed = true;
					cur.mode = CURSOR::MODE::DRAG;
					break;
				// When releasing the mouse remove all selected
				case GW::INPUT::GBufferedInput::Events::BUTTONRELEASED:
					mouseReleased = true;
					cur.mode = CURSOR::MODE::HIGHLIGHT;
					break;
				case GW::INPUT::GBufferedInput::Events::MOUSEMOVE: {
					// record prev 3D cursor pos for shape dragging 
					prev = cir.pos;
					// use the position of the mouse to mathematically  
					// unproject points into the world space.
					GW::MATH::GVECTORF ndc_near = {
						(d.x * 2.0f) / width - 1, 1 - (d.y * 2.0f) / height, 0, 0.01
					};
					GW::MATH::GVECTORF ndc_far = ndc_near;
					ndc_far.z = 1; ndc_far.w = 100; // again, hard-coding not ideal
					// revert perspective divides
					ndc_near.x *= ndc_near.w;
					ndc_near.y *= ndc_near.w;
					ndc_near.z *= ndc_near.w;
					ndc_far.x *= ndc_far.w;
					ndc_far.y *= ndc_far.w;
					ndc_far.z *= ndc_far.w;
					// convert to world
					GW::MATH::GMatrix::VectorXMatrixF(invViewProj, ndc_near, ndc_near);
					GW::MATH::GMatrix::VectorXMatrixF(invViewProj, ndc_far, ndc_far);
					// intersect ray with Z = 0 plane
					float ratio = -ndc_near.z / (ndc_far.z - ndc_near.z);
					// interpolate along plane intersection for X & Y
					cir.pos.x = ndc_near.x + (ndc_far.x - ndc_near.x) * ratio;
					cir.pos.y = ndc_near.y + (ndc_far.y - ndc_near.y) * ratio;
					// first try on this math! not too bad Lari not too bad.
					break;
				}
				case GW::INPUT::GBufferedInput::Events::KEYPRESSED: {
					// orbit camera 10 degrees with each key press
					float spinX = 0, spinZ = 0, distance = 0;
					if (d.data == G_KEY_UP || d.data == G_KEY_NUMPAD_8)
						spinX = 1;
					else if (d.data == G_KEY_DOWN || d.data == G_KEY_NUMPAD_2)
						spinX = -1;
					else if (d.data == G_KEY_LEFT || d.data == G_KEY_NUMPAD_4)
						spinZ = -1;
					else if (d.data == G_KEY_RIGHT || d.data == G_KEY_NUMPAD_6)
						spinZ = 1;
					// get distance from origin
					GW::MATH::GVector::MagnitudeF(renderer.camera.row4.xyz(), distance);
					// place camera at origin
					renderer.camera.row4 = GW::MATH::GIdentityVectorF;
					// apply transformations
					GW::MATH::GMatrix::RotateXLocalF(
						renderer.camera, spinX * G_DEGREE_TO_RADIAN_F(10), renderer.camera);
					GW::MATH::GMatrix::RotateZGlobalF(
						renderer.camera, spinZ * G_DEGREE_TO_RADIAN_F(10), renderer.camera);
					// translate same distance away from origin along -Z
					GW::MATH::GMatrix::TranslateLocalF(
						renderer.camera, GW::MATH::GVECTORF{0,0,-distance,0}, renderer.camera);
					// rebuild combined matricies
					GW::MATH::GMatrix::InverseF(renderer.camera, renderer.viewProjection);
					GW::MATH::GMatrix::MultiplyMatrixF(renderer.viewProjection,
						renderer.projection, renderer.viewProjection);
					break;
				}
				};
			}
		}
		// with all events processed we move to highlighting/selecting any overlapped objects
		// handle highlighting/selection of game objects
		if (cur.mode == CURSOR::MODE::HIGHLIGHT || mousePressed) {
			auto lineView = registry.view<LINE, GAME_OBJECT>();
			for (auto [ent, lin] : lineView.each()) {
				GW::MATH2D::GCollision2D::GCollisionCheck2D result;
				GW::MATH2D::GCollision2D::TestLineToCircle2F(lin, cir, result);
				if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION)
					registry.emplace_or_replace<HIGHLIGHTED>(ent);
			}
			auto rectangleView = registry.view<RECTANGLE, GAME_OBJECT>();
			for (auto [ent, rec] : rectangleView.each()) {
				GW::MATH2D::GCollision2D::GCollisionCheck2D result;
				GW::MATH2D::GCollision2D::TestPointToRectangle2F(cir.pos, rec, result);
				if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION)
					registry.emplace_or_replace<HIGHLIGHTED>(ent);
			}
			auto circleView = registry.view<CIRCLE, GAME_OBJECT>();
			for (auto [ent, circle] : circleView.each()) {
				GW::MATH2D::GCollision2D::GCollisionCheck2D result;
				GW::MATH2D::GCollision2D::TestCircleToCircle2F(cir, circle, result);
				if (result == GW::MATH2D::GCollision2D::GCollisionCheck2D::COLLISION)
					registry.emplace_or_replace<HIGHLIGHTED>(ent);
			}
		}
		if (cur.mode == CURSOR::MODE::DRAG) {
			// upgrade any highlighted items to selected it the mouse was clicked
			auto highlightView = registry.view<HIGHLIGHTED>();
			for (auto [ent] : highlightView.each()) {
				registry.remove<HIGHLIGHTED>(ent);
				registry.emplace_or_replace<SELECTED>(ent);
			}
			// shift any SELECTED objects by the change in cursor position
			auto selectedLines = registry.view<LINE, SELECTED>();
			for (auto [ent, line] : selectedLines.each()) {
				line.start.x += cir.pos.x - prev.x;
				line.start.y += cir.pos.y - prev.y;
				line.end.x += cir.pos.x - prev.x;
				line.end.y += cir.pos.y - prev.y;
			}
			auto selectedRectangles = registry.view<RECTANGLE, SELECTED>();
			for (auto [ent, rectangle] : selectedRectangles.each()) {
				rectangle.min.x += cir.pos.x - prev.x;
				rectangle.min.y += cir.pos.y - prev.y;
				rectangle.max.x += cir.pos.x - prev.x;
				rectangle.max.y += cir.pos.y - prev.y;
			}
			auto selectedCircles = registry.view<CIRCLE, SELECTED>();
			for (auto [ent, circle] : selectedCircles.each()) {
				circle.pos.x += cir.pos.x - prev.x;
				circle.pos.y += cir.pos.y - prev.y;
			}
			// update previous cursor to latest
			prev = cir.pos;
		}
		if (mouseReleased) {
			// wipe any selections
			registry.clear<SELECTED>();
		}
	}
	return true;
}

// Process any renderers used by the game
bool ProcessRenderers(GraphicsEngine& engine) {
	// TODO: process normal rendering systems

	// process debug rendering systems
	auto view = registry.view<DebugRenderer>();
	for (auto entity : view) {
		auto &dr = registry.get<DebugRenderer>(entity);
		// clear previous cpu buffer data
		dr.cpuCache.clear();
		// traverse supported shapes and store dynamic vertex data
		auto lineView = registry.view<LINE, COLOR>();
		for (auto [ent, lin, clr] : lineView.each()) {
			dr.cpuCache.push_back({ lin.start, clr });
			dr.cpuCache.push_back({ lin.end, clr });
		}
		auto rectView = registry.view<RECTANGLE, COLOR>();
		for (auto [ent, rct, clr] : rectView.each()) {
			dr.cpuCache.push_back({ rct.min, clr });
			dr.cpuCache.push_back({ { rct.min.x, rct.max.y }, clr });
			dr.cpuCache.push_back({ rct.min, clr });
			dr.cpuCache.push_back({ { rct.max.x, rct.min.y }, clr });
			dr.cpuCache.push_back({ rct.max, clr });
			dr.cpuCache.push_back({ { rct.max.x, rct.min.y }, clr });
			dr.cpuCache.push_back({ rct.max, clr });
			dr.cpuCache.push_back({ { rct.min.x, rct.max.y }, clr });
		}
		auto circleView = registry.view<CIRCLE, COLOR>();
		for (auto [ent, cir, clr] : circleView.each()) {
			float step = G_PI_F * 2 / 36;
			for (float rad = 0; rad <= G_PI_F * 2;) {
				dr.cpuCache.push_back({ { cir.pos.x + cosf(rad) * cir.radius,
					 cir.pos.y + sinf(rad) * cir.radius }, clr });
				rad += step; // move along circle
				dr.cpuCache.push_back({ { cir.pos.x + cosf(rad) * cir.radius,
					 cir.pos.y + sinf(rad) * cir.radius }, clr });
			}
		}
		// if the GPU buffer is missing or too small, we allocate more room
		if (dr.gpuVertexCapacity < dr.cpuCache.size()) {
			ID3D11Device* creator;
			if (-engine.surface.GetDevice((void**)&creator))
				return false;
			dr.vertexBuffer.Reset();
			CD3D11_BUFFER_DESC bDesc(sizeof(DebugRenderer::debug_vertex) * dr.cpuCache.size(),
				D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
			creator->CreateBuffer(&bDesc, nullptr, dr.vertexBuffer.GetAddressOf());
			creator->Release(); // done allocating, should have enough room now
			// update maximum internal capacity
			dr.gpuVertexCapacity = dr.cpuCache.size();
		}
		// transmit CPU data to GPU
		ID3D11DeviceContext* con;
		if (-engine.surface.GetImmediateContext((void**)&con))
			return false;
		D3D11_MAPPED_SUBRESOURCE gpuBuffer;
		con->Map(dr.vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &gpuBuffer);
			memcpy(	gpuBuffer.pData, dr.cpuCache.data(), 
					sizeof(DebugRenderer::debug_vertex) * dr.cpuCache.size());
		con->Unmap(dr.vertexBuffer.Get(), 0);
		// render all shape data
		ID3D11RenderTargetView* view;
		ID3D11DepthStencilView* depth;
		engine.surface.GetRenderTargetView((void**)&view);
		engine.surface.GetDepthStencilView((void**)&depth);
		// setup the pipeline
		ID3D11RenderTargetView* const views[] = { view };
		con->OMSetRenderTargets(ARRAYSIZE(views), views, depth);
		const UINT strides[] = { sizeof(DebugRenderer::debug_vertex) };
		const UINT offsets[] = { 0 };
		ID3D11Buffer* const buffs[] = { dr.vertexBuffer.Get() };
		con->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
		con->VSSetShader(dr.vertexShader.Get(), nullptr, 0);
		con->PSSetShader(dr.pixelShader.Get(), nullptr, 0);
		con->IASetInputLayout(dr.vertexFormat.Get());
		// set and update the constant buffer (cb)
		con->VSSetConstantBuffers(0, 1, dr.constantBuffer.GetAddressOf());
		con->UpdateSubresource(dr.constantBuffer.Get(), 0, nullptr, &dr.viewProjection, 0, 0);
		// now we can draw
		con->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		con->Draw(dr.cpuCache.size(), 0);
		// release temp handles
		depth->Release();
		view->Release();
		con->Release();
	}
	return true;
}

// runs the graphics for the game
bool ProcessGraphics() {
	auto view = registry.view<GraphicsEngine>();
	for (auto entity : view) {
		auto &ge = registry.get<GraphicsEngine>(entity);
		// grab d3d11 handles, clear screen, run rendererers
		IDXGISwapChain* swap;
		ID3D11DeviceContext* con;
		ID3D11RenderTargetView* view;
		ID3D11DepthStencilView* depth;
		if (+ge.surface.GetImmediateContext((void**)&con) &&
			+ge.surface.GetRenderTargetView((void**)&view) &&
			+ge.surface.GetDepthStencilView((void**)&depth) &&
			+ge.surface.GetSwapchain((void**)&swap))
		{
			con->ClearRenderTargetView(view, ge.clear_color);
			con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, ge.clear_depth, 0);

			// run all rendering systems here
			if (ProcessRenderers(ge) == false)
				return false; // failure

			swap->Present(0, 0); // present final results
			// release incremented COM reference counts
			swap->Release();
			view->Release();
			depth->Release();
			con->Release();
		}
		else
			return false;
	}
	return true;
}

// runs the main game loop
int ProcessGameLoop() {
	auto view = registry.view<GameLoop>();
	for (auto entity : view) {
		auto &gl = registry.get<GameLoop>(entity);
		// listen for ESC key press (temporary for development)
		GW::INPUT::GInput input; 
		if (-input.Create(gl.window)) 
			return false;
		// loop processing all other systems
		while (+gl.window.ProcessWindowEvents()) {
			if (ProcessCursors(gl) == false)
				return 1; // failure
			if (ProcessGameObjects() == false)
				return 1; // failure
			if (ProcessColors() == false)
				return 1; // failure
			if (ProcessGraphics() == false)
				return 1; // failure
			// check if the user wants to quit (replace with main menu)
			float press = 0;
			if (+input.GetState(G_KEY_ESCAPE, press) && press > 0) {
				// destroy window immediately
				gl.window = nullptr; // prevents crash on exit
				return 0; // successfully leave
			}
		}
	}
	return 0; // success for main()
}