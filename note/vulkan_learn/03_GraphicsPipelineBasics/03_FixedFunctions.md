# Các hàm cố định (Fixed functions)

Các API đồ họa cũ hơn thường cung cấp trạng thái mặc định (default state) cho hầu hết các giai đoạn trong graphics pipeline. Trong Vulkan, bạn phải chỉ định rõ ràng hầu hết các trạng thái của pipeline, vì chúng sẽ được "nướng chín" (baked) vào trong một đối tượng trạng thái pipeline bất biến (immutable pipeline state object). Trong chương này, chúng ta sẽ cấu hình tất cả các cấu trúc (struct) cần thiết cho các hoạt động cố định này.

---

## Trạng thái động (Dynamic state)

Mặc dù phần lớn trạng thái của pipeline cần phải được đóng gói cố định (baked) vào pipeline, một số lượng giới hạn các trạng thái vẫn có thể thay đổi trong thời điểm vẽ (draw time) mà không cần phải tạo lại pipeline. Ví dụ như kích thước của viewport, độ dày đường thẳng (line width) và các hằng số pha trộn màu (blend constants). Nếu bạn muốn sử dụng dynamic state và không cố định các thuộc tính này vào pipeline, bạn sẽ cần điền vào cấu trúc `vk::PipelineDynamicStateCreateInfo` như sau:

```cpp
std::vector<vk::DynamicState> dynamicStates = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
};

vk::PipelineDynamicStateCreateInfo dynamicState{
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates    = dynamicStates.data()
};
```

Thiết lập này sẽ khiến các cấu hình tĩnh tương ứng trong pipeline bị bỏ qua, và bạn sẽ có thể (đồng thời bắt buộc) phải chỉ định dữ liệu của chúng vào thời điểm vẽ (drawing time). Điều này mang lại sự linh hoạt cao hơn và rất phổ biến cho các trạng thái như viewport và scissor, vốn sẽ trở nên rất cồng kềnh và phức tạp nếu bị đóng cứng (baked) vào trạng thái của pipeline.

---

## Đầu vào đỉnh (Vertex input)

Cấu trúc `vk::PipelineVertexInputStateCreateInfo` mô tả định dạng của dữ liệu đỉnh (vertex data) sẽ được truyền vào vertex shader. Nó mô tả điều này theo hai khía cạnh chính:

* **Bindings**: Khoảng cách giữa các phần dữ liệu (stride/spacing) và việc dữ liệu đó là trên từng đỉnh (per-vertex) hay trên từng instance (per-instance, xem thêm về instancing).
* **Attribute descriptions**: Kiểu dữ liệu của các thuộc tính truyền vào vertex shader, nạp chúng từ binding nào và ở độ lệch (offset) là bao nhiêu.

Vì hiện tại chúng ta đang hardcode (viết cứng) dữ liệu đỉnh trực tiếp bên trong vertex shader, chúng ta sẽ chỉ cần điền cấu trúc này để chỉ định rằng tạm thời chưa có dữ liệu đỉnh nào cần nạp. Chúng ta sẽ quay lại phần này trong chương về Vertex Buffer.

```cpp
vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
```

Các thành viên `pVertexBindingDescriptions` và `pVertexAttributeDescriptions` trỏ đến mảng các struct mô tả chi tiết cách nạp dữ liệu đỉnh đã đề cập ở trên. Hãy thêm cấu trúc này vào hàm `createGraphicsPipeline` ngay sau mảng `shaderStages`.

---

## Lắp ghép hình học (Input assembly)

Cấu trúc `vk::PipelineInputAssemblyStateCreateInfo` mô tả hai điều: loại hình học nào sẽ được dựng từ các đỉnh và liệu có kích hoạt tính năng khởi động lại nguyên thủy (primitive restart) hay không. Loại hình học được chỉ định trong trường `topology` và có thể nhận các giá trị như:

* `vk::PrimitiveTopology::ePointList`: Các điểm rời rạc từ các đỉnh.
* `vk::PrimitiveTopology::eLineList`: Đoạn thẳng từ mỗi 2 đỉnh liên tiếp (không tái sử dụng đỉnh).
* `vk::PrimitiveTopology::eLineStrip`: Đỉnh kết thúc của đoạn thẳng này được dùng làm đỉnh bắt đầu của đoạn thẳng tiếp theo.
* `vk::PrimitiveTopology::eTriangleList`: Hình tam giác từ mỗi 3 đỉnh liên tiếp (không tái sử dụng đỉnh).
* `vk::PrimitiveTopology::eTriangleStrip`: Đỉnh thứ hai và thứ ba của mỗi tam giác được tái sử dụng làm hai đỉnh đầu tiên của tam giác tiếp theo.

Thông thường, các đỉnh được nạp tuần tự từ vertex buffer theo thứ tự chỉ mục (index). Tuy nhiên, với một element buffer (index buffer), bạn có thể tự mình chỉ định các chỉ số index cần dùng. Điều này cho phép thực hiện các tối ưu hóa như tái sử dụng lại các đỉnh. Nếu bạn đặt thành viên `primitiveRestartEnable` thành `vk::True`, bạn có thể ngắt các đường nối hoặc chuỗi tam giác trong các chế độ topology dạng `Strip` bằng cách sử dụng một index đặc biệt là `0xFFFF` hoặc `0xFFFFFFFF`.

Chúng ta dự định sẽ vẽ các hình tam giác trong suốt loạt bài hướng dẫn này, vì vậy chúng ta sẽ giữ nguyên cấu hình sau:

```cpp
vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
    .topology = vk::PrimitiveTopology::eTriangleList
};
```

---

## Vùng nhìn và Vùng cắt (Viewports and scissors)

Một **viewport** cơ bản mô tả vùng của framebuffer mà hình ảnh đầu ra sẽ được render vào. Vùng này hầu như luôn là từ `(0, 0)` đến `(width, height)`, và trong hướng dẫn này cũng sẽ như vậy.

```cpp
vk::Viewport viewport{
    0.0f, 0.0f,
    static_cast<float>(swapChainExtent.width),
    static_cast<float>(swapChainExtent.height),
    0.0f, 1.0f
};
```

Lưu ý rằng kích thước của swap chain và các hình ảnh của nó có thể khác với `WIDTH` và `HEIGHT` của cửa sổ. Các hình ảnh của swap chain sau này sẽ được dùng làm framebuffers, do đó chúng ta nên tuân theo kích thước của chúng.

Các giá trị `minDepth` và `maxDepth` chỉ định phạm vi giá trị độ sâu (depth) được sử dụng cho framebuffer. Các giá trị này phải nằm trong khoảng `[0.0f, 1.0f]`, nhưng `minDepth` có thể lớn hơn `maxDepth`. Nếu không làm điều gì đặc biệt, bạn nên giữ nguyên các giá trị tiêu chuẩn là `0.0f` và `1.0f`.

Trong khi viewport xác định phép biến đổi (transformation) từ hình ảnh sang framebuffer, thì các **hình chữ nhật cắt (scissor rectangles)** lại xác định vùng pixel thực tế nào sẽ được lưu trữ. Bộ rasterizer sẽ vứt bỏ (discard) bất kỳ pixel nào nằm ngoài các hình chữ nhật scissor. Chúng hoạt động giống như một bộ lọc (filter) hơn là một phép biến đổi. Điểm khác biệt này được minh họa qua việc scissor như một mặt nạ lọc pixel: bất kỳ pixel nào ngoài vùng scissor sẽ bị loại bỏ, miễn là vùng scissor bao phủ vùng bạn muốn hiển thị.

Vì vậy, nếu muốn vẽ lên toàn bộ framebuffer, chúng ta sẽ chỉ định một hình chữ nhật scissor bao phủ toàn bộ nó:

```cpp
vk::Rect2D scissor{vk::Offset2D{ 0, 0 }, swapChainExtent};
```

Viewport và scissor rectangle có thể được chỉ định cố định (static) như một phần của pipeline, hoặc dưới dạng trạng thái động (dynamic state) được thiết lập trong command buffer. Mặc dù cách tĩnh đồng nhất hơn với các trạng thái khác, nhưng việc đặt viewport và scissor thành dynamic thường thuận tiện hơn vì nó mang lại độ linh hoạt cao. Cách làm này rất phổ biến và mọi phần cứng hiện nay đều hỗ trợ dynamic state này mà không làm giảm hiệu năng.

Khi lựa chọn dynamic viewport và scissor rectangle, bạn cần bật các trạng thái dynamic tương ứng cho pipeline:

```cpp
std::vector<vk::DynamicState> dynamicStates = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
};

vk::PipelineDynamicStateCreateInfo dynamicState{
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates    = dynamicStates.data()
};
```

Và sau đó bạn chỉ cần chỉ định số lượng của chúng tại thời điểm tạo pipeline:

```cpp
vk::PipelineViewportStateCreateInfo viewportState{
    .viewportCount = 1,
    .scissorCount  = 1
};
```

Các viewport và scissor rectangle thực tế sau đó sẽ được thiết lập trong thời điểm thực hiện lệnh vẽ (drawing time).

Với dynamic state, bạn thậm chí có thể chỉ định các viewport và/hoặc scissor rectangle khác nhau ngay bên trong cùng một command buffer.

Nếu không dùng dynamic state, viewport và scissor rectangle cần phải được gán cứng vào pipeline bằng cấu trúc `vk::PipelineViewportStateCreateInfo`. Điều này làm cho viewport và scissor của pipeline đó trở nên bất biến. Bất kỳ thay đổi nào đối với các giá trị này đều sẽ đòi hỏi phải tạo lại một pipeline mới với các giá trị mới:

```cpp
vk::PipelineViewportStateCreateInfo viewportState{
    .viewportCount = 1,
    .pViewports    = &viewport,
    .scissorCount  = 1,
    .pScissors     = &scissor
};
```

Dù bạn thiết lập theo cách nào, một số card đồ họa hỗ trợ sử dụng nhiều viewport và scissor rectangle cùng lúc, vì thế các trường của struct tham chiếu tới một mảng chứa chúng. Việc sử dụng nhiều viewport đòi hỏi phải bật một tính năng GPU tương ứng (xem phần tạo logical device).

---

## Bộ raster hóa (Rasterizer)

Bộ rasterizer tiếp nhận hình học được định hình từ các đỉnh bởi vertex shader và biến đổi chúng thành các mảnh điểm ảnh (fragments) để fragment shader tô màu. Nó cũng thực hiện kiểm tra độ sâu (depth testing), loại bỏ mặt khuất (face culling) và kiểm tra cắt (scissor test), đồng thời có thể được cấu hình để xuất ra các fragment lấp đầy toàn bộ đa giác hoặc chỉ vẽ các cạnh (wireframe rendering). Tất cả những điều này được cấu hình bằng cấu trúc `vk::PipelineRasterizationStateCreateInfo`.

```cpp
vk::PipelineRasterizationStateCreateInfo rasterizer{
    .depthClampEnable        = vk::False,
    .rasterizerDiscardEnable = vk::False,
    .polygonMode             = vk::PolygonMode::eFill,
    .cullMode                = vk::CullModeFlagBits::eBack,
    .frontFace               = vk::FrontFace::eClockwise,
    .depthBiasEnable         = vk::False,
    .lineWidth               = 1.0f
};
```

* Nếu `depthClampEnable` được đặt thành `vk::True`, các fragment vượt quá mặt phẳng gần (near plane) và xa (far plane) sẽ được ghim lại (clamp) vào các mặt phẳng đó thay vì bị loại bỏ. Điều này hữu ích trong một số trường hợp đặc biệt như shadow maps (bản đồ đổ bóng). Sử dụng tính năng này yêu cầu phải bật tính năng tương ứng của GPU.
* Nếu `rasterizerDiscardEnable` được đặt thành `vk::True`, hình học sẽ không bao giờ đi qua giai đoạn rasterizer. Điều này về cơ bản sẽ vô hiệu hóa mọi đầu ra ghi vào framebuffer.
* Trường `polygonMode` xác định cách các fragment được tạo ra cho hình học. Có các chế độ sau:
  * `vk::PolygonMode::eFill`: Lấp đầy toàn bộ diện tích của đa giác bằng các fragment.
  * `vk::PolygonMode::eLine`: Các cạnh của đa giác được vẽ thành các đường thẳng (wireframe).
  * `vk::PolygonMode::ePoint`: Các đỉnh của đa giác được vẽ thành các điểm.
  *(Sử dụng bất kỳ chế độ nào khác ngoài `fill` đều yêu cầu bật tính năng tương ứng của GPU).*
* Biến `cullMode` xác định kiểu loại bỏ mặt khuất (face culling). Bạn có thể tắt culling, loại bỏ mặt trước, loại bỏ mặt sau, hoặc loại bỏ cả hai.
* Biến `frontFace` chỉ định thứ tự các đỉnh để một mặt được coi là mặt trước (front-facing), có thể là theo chiều kim đồng hồ (`eClockwise`) hoặc ngược chiều kim đồng hồ (`eCounterClockwise`).
* Bộ rasterizer có thể thay đổi các giá trị độ sâu bằng cách thêm vào một hằng số hoặc làm lệch (bias) dựa trên độ dốc của fragment. Điều này đôi khi được dùng trong kỹ thuật shadow mapping, nhưng chúng ta sẽ không dùng tới. Chỉ cần đặt `depthBiasEnable` thành `vk::False`.
* Thành viên `lineWidth` khá đơn giản, nó mô tả độ dày của các đường thẳng theo số lượng fragment. Độ dày tối đa được hỗ trợ phụ thuộc vào phần cứng, và bất kỳ đường kẻ nào dày hơn `1.0f` đều yêu cầu bạn phải kích hoạt tính năng `wideLines` của GPU.

---

## Khử răng cưa đa mẫu (Multisampling)

Cấu trúc `vk::PipelineMultisampleStateCreateInfo` cấu hình multisampling (MSAA), một trong những phương pháp khử răng cưa (anti-aliasing). Nó hoạt động bằng cách kết hợp kết quả fragment shader của nhiều đa giác cùng rasterize vào cùng một pixel. Hiện tượng răng cưa chủ yếu xuất hiện dọc theo các cạnh viền, và đây cũng là nơi các lỗi răng cưa dễ nhận thấy nhất. Vì không cần phải chạy fragment shader nhiều lần nếu chỉ có một đa giác bao phủ pixel đó, multisampling ít tốn kém tài nguyên hơn đáng kể so với việc render ở độ phân giải cao hơn rồi thu nhỏ lại (supersampling). Việc bật tính năng này yêu cầu kích hoạt GPU feature tương ứng.

```cpp
vk::PipelineMultisampleStateCreateInfo multisampling{
    .rasterizationSamples = vk::SampleCountFlagBits::e1,
    .sampleShadingEnable  = vk::False
};
```

Chúng ta sẽ tìm hiểu lại về multisampling trong một chương sau; hiện tại chúng ta cứ giữ nó ở trạng thái tắt.

---

## Kiểm tra độ sâu và khuôn (Depth and stencil testing)

Nếu bạn sử dụng depth buffer và/hoặc stencil buffer, bạn cũng cần cấu hình kiểm tra depth và stencil bằng cách sử dụng `vk::PipelineDepthStencilStateCreateInfo`. Hiện tại chúng ta chưa dùng đến, do đó chúng ta có thể truyền một con trỏ `nullptr` thay vì con trỏ tới struct này. Chúng ta sẽ quay lại phần này trong chương về depth buffering.

---

## Pha trộn màu (Color blending)

Sau khi fragment shader trả về một màu sắc, nó cần được kết hợp với màu đã có sẵn trong framebuffer. Quá trình biến đổi này được gọi là **color blending (pha trộn màu)**, và có hai cách để thực hiện:

1. Trộn giá trị cũ và giá trị mới để tạo ra một màu cuối cùng.
2. Kết hợp giá trị cũ và giá trị mới bằng một phép toán thao tác bit (bitwise operation).

Có hai loại struct để cấu hình color blending:
* Struct thứ nhất: `vk::PipelineColorBlendAttachmentState` chứa cấu hình riêng cho từng framebuffer đính kèm (per attached framebuffer).
* Struct thứ hai: `vk::PipelineColorBlendStateCreateInfo` chứa các thiết lập color blending toàn cục (global).

Trong trường hợp của chúng ta, chúng ta chỉ có một framebuffer duy nhất:

```cpp
vk::PipelineColorBlendAttachmentState colorBlendAttachment{
    .blendEnable    = vk::False,
    .colorWriteMask = vk::ColorComponentFlagBits::eR |
                      vk::ColorComponentFlagBits::eG |
                      vk::ColorComponentFlagBits::eB |
                      vk::ColorComponentFlagBits::eA
};
```

Struct riêng cho từng framebuffer này cho phép bạn cấu hình phương pháp pha trộn màu thứ nhất. Các phép toán sẽ được thực hiện được biểu diễn rõ nhất qua đoạn mã giả (pseudocode) sau:

```cpp
if (blendEnable) {
    finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
    finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
} else {
    finalColor = newColor;
}

finalColor = finalColor & colorWriteMask;
```

Nếu `blendEnable` được đặt là `vk::False`, thì màu mới từ fragment shader sẽ được truyền qua mà không bị chỉnh sửa. Ngược lại, hai phép toán trộn sẽ được thực thi để tính toán màu mới. Màu kết quả sẽ được thực hiện phép toán `AND` với `colorWriteMask` để xác định những kênh màu nào thực sự được ghi lại.

Cách phổ biến nhất khi sử dụng color blending là triển khai **alpha blending**, nơi màu mới được pha trộn với màu cũ dựa trên độ trong suốt (opacity) của nó. Khi đó `finalColor` sẽ được tính toán như sau:

```
finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
finalColor.a = newAlpha.a;
```

Công thức này có thể đạt được với các tham số sau:

```cpp
vk::PipelineColorBlendAttachmentState colorBlendAttachment{
    .blendEnable         = vk::True,
    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .colorBlendOp        = vk::BlendOp::eAdd,
    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
    .alphaBlendOp        = vk::BlendOp::eAdd,
    .colorWriteMask      = vk::ColorComponentFlagBits::eR |
                           vk::ColorComponentFlagBits::eG |
                           vk::ColorComponentFlagBits::eB |
                           vk::ColorComponentFlagBits::eA
};
```

Bạn có thể tìm thấy tất cả các toán tử khả dụng trong các enum `vk::BlendFactor` và `vk::BlendOp` trong tài liệu đặc tả của Vulkan.

Cấu trúc thứ hai tham chiếu đến mảng các struct cho tất cả các framebuffer và cho phép bạn đặt các hằng số blend (blend constants) có thể được sử dụng làm các hệ số pha trộn (blend factors) trong các phép tính nêu trên.

```cpp
vk::PipelineColorBlendStateCreateInfo colorBlending{
    .logicOpEnable   = vk::False,
    .logicOp         = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments    = &colorBlendAttachment
};
```

Nếu bạn muốn sử dụng phương pháp pha trộn thứ hai (kết hợp theo bit - bitwise combination), bạn cần đặt `logicOpEnable` thành `vk::True`. Phép toán bit sau đó có thể được chỉ định trong trường `logicOp`. Lưu ý rằng điều này sẽ tự động vô hiệu hóa phương pháp thứ nhất, tương tự như việc bạn đã đặt `blendEnable = vk::False` cho tất cả các framebuffer được đính kèm! Trường `colorWriteMask` cũng sẽ được sử dụng trong chế độ này để xác định kênh màu nào trong framebuffer thực sự bị ảnh hưởng. Bạn cũng có thể tắt cả hai chế độ (như chúng ta đang làm ở đây), trong trường hợp đó các màu của fragment sẽ được ghi thẳng vào framebuffer mà không có bất kỳ sửa đổi nào.

---

## Bố cục Pipeline (Pipeline layout)

Bạn có thể sử dụng các giá trị **uniform** trong shaders, đây là các biến toàn cục (tương tự như các biến trạng thái động) có thể thay đổi tại thời điểm vẽ (draw time) để điều chỉnh hành vi của shader mà không cần phải tạo lại chúng. Chúng thường được dùng để truyền ma trận biến đổi (transformation matrix) cho vertex shader, hoặc tạo texture sampler trong fragment shader.

Các giá trị uniform này cần được khai báo trong quá trình tạo pipeline thông qua việc khởi tạo một đối tượng `vk::PipelineLayout`. Mặc dù chúng ta chưa dùng đến chúng cho tới các chương sau, chúng ta vẫn bắt buộc phải tạo một pipeline layout rỗng.

Hãy tạo một biến thành viên trong class để lưu trữ đối tượng này, vì chúng ta sẽ cần tham chiếu đến nó từ các hàm khác ở phần sau:

```cpp
vk::raii::PipelineLayout pipelineLayout = nullptr;
```

Và sau đó tạo đối tượng này trong hàm `createGraphicsPipeline`:

```cpp
vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
    .setLayoutCount         = 0,
    .pushConstantRangeCount = 0
};

pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
```

Cấu trúc này cũng cho phép chỉ định **push constants**, một cách khác để truyền các giá trị động sang shader mà chúng ta có thể sẽ tìm hiểu trong một chương sau.

---

## Kết luận (Conclusion)

Đó là toàn bộ cấu hình cho các trạng thái fixed-function! Việc thiết lập tất cả từ đầu đòi hỏi khá nhiều công sức, nhưng ưu điểm là giờ đây chúng ta đã nắm bắt và kiểm soát được gần như toàn bộ những gì đang diễn ra bên trong graphics pipeline! Điều này giúp giảm thiểu nguy cơ gặp phải các hành vi bất thường xuất phát từ việc các trạng thái mặc định của phần cứng không như mong đợi.

Ở bước tiếp theo, chúng ta sẽ thiết lập **dynamic rendering** để thông báo cho graphics pipeline biết những attachment nào sẽ được sử dụng và sử dụng ra sao.
